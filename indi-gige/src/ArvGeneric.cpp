/*
 GigE interface wrapper on araviss
 Copyright (C) 2016 Hendrik Beijeman (hbeyeman@gmail.com)
 Copyright (C) 2021 Linus Molteno (linus@molteno.net)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "ArvGeneric.h"
#include "indi_gige.h"

using namespace arv;

const char *ArvGeneric::_str_val(const char *s)
{
    return (s ? s : "None");
}
const char *ArvGeneric::vendor_name()
{
    return this->_str_val(this->cam.vendor_name);
}
const char *ArvGeneric::model_name()
{
    return this->_str_val(this->cam.model_name);
}
const char *ArvGeneric::device_id()
{
    return this->_str_val(this->cam.device_id);
}
min_max_property<int> ArvGeneric::get_bin_x()
{
    return min_max_property<int>(this->cam.bin_x);
}
min_max_property<int> ArvGeneric::get_bin_y()
{
    return min_max_property<int>(this->cam.bin_y);
}
min_max_property<int> ArvGeneric::get_x_offset()
{
    return min_max_property<int>(this->cam.x_offset);
}
min_max_property<int> ArvGeneric::get_y_offset()
{
    return min_max_property<int>(this->cam.y_offset);
}
min_max_property<int> ArvGeneric::get_width()
{
    return min_max_property<int>(this->cam.width);
}
min_max_property<int> ArvGeneric::get_height()
{
    return min_max_property<int>(this->cam.height);
}
min_max_property<int> ArvGeneric::get_bpp()
{
    return min_max_property<int>(8,8,8);
}
min_max_property<double> ArvGeneric::get_pixel_pitch()
{
    return min_max_property<double>(this->cam.pixel_pitch);
}
min_max_property<double> ArvGeneric::get_exposure()
{
    return min_max_property<double>(this->cam.exposure);
}
min_max_property<double> ArvGeneric::get_gain()
{
    return min_max_property<double>(this->cam.gain);
}
min_max_property<double> ArvGeneric::get_frame_rate()
{
    return min_max_property<double>(this->cam.frame_rate);
}

template <typename T>
bool ArvGeneric::_get_bounds(void (*fn_arv_bounds)(::ArvCamera *, T *min, T *max, GError** err), min_max_property<T> *prop)
{
    T min, max;
    GError *error = nullptr;
    fn_arv_bounds(this->camera, &min, &max, &error);
    if (error) { printf("Encountered error setting bounds"); g_clear_error(&error); }
    prop->update(min, max);
    return true;
}

bool ArvGeneric::is_exposing()
{
    return this->stream_active;
}
bool ArvGeneric::is_connected()
{
    return (this->camera ? true : false);
}
bool ArvGeneric::_stream_active()
{
    return this->stream_active;
}

void ArvGeneric::updateINDIpointer(GigECCD *indi)
{
    this->indiccd = indi;
}

ArvGeneric::ArvGeneric(void *camera_device) : ArvCamera(camera_device)
{
    GError *error = nullptr;
    this->_init();
    this->camera  = (::ArvCamera *)camera_device;
    this->dev = arv_camera_get_device(this->camera);
    this->reset_camera();

    if (!error) { this->cam.model_name  = arv_camera_get_model_name(this->camera, &error); }
    if (!error) { this->cam.vendor_name = arv_camera_get_vendor_name(this->camera, &error); }
    if (!error) { this->cam.device_id   = arv_camera_get_device_id(this->camera, &error); }

    

    // please don't be mad at me for doing this, it's the only way i could get it to work
    gint xmin, xmax;
    gint ymin, ymax;
    gint xoffmin, xoffmax;
    gint yoffmin, yoffmax;
    gint wmax;
    gint hmax;
    double frmin, frmax;
    double expmin, expmax;
    double gainmin, gainmax;
    if (!error) { arv_camera_get_x_binning_bounds(this->camera, &xmin, &xmax, &error); }
    if (!error) { arv_camera_get_y_binning_bounds(this->camera, &ymin, &ymax, &error); } 
    if (!error) { arv_camera_get_x_offset_bounds(this->camera, &xoffmin, &xoffmax, &error); } 
    if (!error) { arv_camera_get_y_offset_bounds(this->camera, &yoffmin, &yoffmax, &error); }
    if (!error) { arv_camera_get_sensor_size(this->camera, &wmax, &hmax, &error); } 

//    arv_camera_get_width_bounds(this->camera, &wmin, &wmax);
//    arv_camera_get_height_bounds(this->camera, &hmin, &hmax);

    if (!error) { arv_camera_get_frame_rate_bounds(this->camera, &frmin, &frmax, &error); }
    if (!error) { arv_camera_get_exposure_time_bounds(this->camera, &expmin, &expmax, &error); }
    if (!error) { arv_camera_get_gain_bounds(this->camera, &gainmin, &gainmax, &error); }

    this->cam.bin_x.update(xmin, xmax);
    this->cam.bin_y.update(ymin, ymax);
    this->cam.x_offset.update(xoffmin, xoffmax);
    this->cam.y_offset.update(yoffmin, yoffmax);
    this->cam.width.update(0, wmax);
    this->cam.height.update(0, hmax);
    this->cam.frame_rate.update(frmin, frmax);
    this->cam.exposure.update(expmin, expmax);
    this->cam.gain.update(gainmin, gainmax);
    this->cam.pixel_pitch.set_single(5.5);

    printf("Finished Generic Constructor");

    if (error) {
        printf("Encountered error setting up!");
        g_clear_error(&error); 
    }

}

ArvGeneric::~ArvGeneric()
{
    if (this->is_connected())
    {
        this->disconnect();
    }
    this->_init();
}

bool ArvGeneric::connect()
{
    /* (Re-)connect by means of the device-id */
    if (!this->camera)
    {
        GError *error = nullptr;
        if (!error) { this->camera = ::arv_camera_new(this->cam.device_id, &error); }
        if (!this->camera)
            return false;

        this->dev = arv_camera_get_device(this->camera);
        if (!error) { this->cam.model_name  = arv_camera_get_model_name(this->camera, &error); }
        if (!error) { this->cam.vendor_name = arv_camera_get_vendor_name(this->camera, &error); }
        if (!error) { this->cam.device_id   = arv_camera_get_device_id(this->camera, &error); }

        if (error) {
            printf("Encountered error reconnecting!");
            g_clear_error(&error); 
        }
    }
    printf("Set up!");
    this->_configure();

    this->indiccd->LogString("Connected");
    return true;
}

bool ArvGeneric::_configure(void)
{
    this->_set_initial_config();
    return this->_get_initial_config();
}

void ArvGeneric::_init()
{
    this->camera        = nullptr;
    this->buffer        = nullptr;
    this->stream        = nullptr;
    this->stream_active = false;

    /* Don't clear device_id, its needed to re-attach with connect() */
}

bool ArvGeneric::disconnect()
{
    if (this->is_connected())
    {
        this->_test_exposure_and_abort();
        g_clear_object(&this->camera);
    }
    this->_init();
    return true;
}

bool ArvGeneric::_set_initial_config()
{
    /* Configure "manual" mode
     *      (1) disable auto exposure
     *      (2) disable auto framerate (to enable maximum possible exposure time)
     *      (3) set binning to 1x1
     *      (4) set software trigger */
    
    GError *error = nullptr;

    if (!error) { arv_camera_set_binning(camera, 1, 1, &error); }
    if (!error) { arv_camera_set_gain_auto(camera, ARV_AUTO_OFF, &error); }
    if (!error) { arv_camera_set_exposure_time_auto(camera, ARV_AUTO_OFF, &error); }
    if (!error) { arv_camera_set_trigger(camera, "Software", &error); }

/*
    guint n_pixel_formats = 10;
    const char ** formats = arv_camera_dup_available_pixel_formats_as_display_names(this->camera, &n_pixel_formats, NULL);
    for (int i = 0; i < n_pixel_formats; i++) {
        this->indiccd->LogString(*(formats+i));
    }

    const char * format = arv_camera_get_pixel_format_as_string(this->camera, NULL); 
    this->indiccd->LogString(format);
    this->indiccd->LogString("But we currently have");
*/

    if (error) { this->indiccd->LogString("Encountered error in setting initial config!"); g_clear_error(&error); } 
    this->indiccd->LogString("Made it through setting initial config!");
    return true;
}

bool ArvGeneric::_get_initial_config()
{
    this->_get_bounds<gint>(arv_camera_get_x_binning_bounds, &this->cam.bin_x);
    this->_get_bounds<gint>(arv_camera_get_y_binning_bounds, &this->cam.bin_y);
    this->_get_bounds<gint>(arv_camera_get_x_offset_bounds, &this->cam.x_offset);
    this->_get_bounds<gint>(arv_camera_get_y_offset_bounds, &this->cam.y_offset);
    this->_get_bounds<gint>(arv_camera_get_width_bounds, &this->cam.width);
    this->_get_bounds<gint>(arv_camera_get_height_bounds, &this->cam.height);
    this->_get_bounds<double>(arv_camera_get_frame_rate_bounds, &this->cam.frame_rate);
    this->_get_bounds<double>(arv_camera_get_exposure_time_bounds, &this->cam.exposure);
    this->_get_bounds<double>(arv_camera_get_gain_bounds, &this->cam.gain);

    /* No GVCP call for this..., specialize if necessary */
    this->cam.pixel_pitch.set_single(5.5);


    GError *error = nullptr;
    if (!error) { this->cam.vendor_name = arv_camera_get_vendor_name(camera, &error); }
    if (!error) { this->cam.model_name  = arv_camera_get_model_name(camera, &error); }
    if (!error) { this->cam.device_id   = arv_camera_get_device_id(camera, &error); }

    if (error) { printf("Encountered error!"); g_clear_error(&error); return false; }
    return true;
}

int ArvGeneric::get_frame_byte_size()
{
    GError *error = nullptr;
    int payload = arv_camera_get_payload(this->camera, &error);
    if (error) { printf("Couldn't get frame byte size!"); g_clear_error(&error); return -1;}
    return payload;
}

void ArvGeneric::set_geometry(int const x, int const y, int const w, int const h)
{
    this->cam.x_offset.set(x);
    this->cam.y_offset.set(y);
    this->cam.width.set(w);
    this->cam.height.set(h);

    GError *error = nullptr;
    if (!error) { arv_camera_set_region(this->camera, this->cam.x_offset.val(), this->cam.y_offset.val(),
            this->cam.width.val(), this->cam.height.val(), &error); }
    if (error) { printf("Encountered error setting region!"); g_clear_error(&error); }
}

void ArvGeneric::update_geometry(void)
{
    gint x, y, w, h, binx, biny;

    GError *error = nullptr;
    if (!error) { arv_camera_get_region(this->camera, &x, &y, &w, &h, &error); }
    if (!error) { arv_camera_get_binning(this->camera, &binx, &biny, &error); }

    if (error) { this->indiccd->LogString("Encountered error updating geometry!"); g_clear_error(&error); }

    this->cam.x_offset.set(x);
    this->cam.y_offset.set(y);
    this->cam.width.set(w);
    this->cam.height.set(h);
    this->cam.bin_x.set(binx);
    this->cam.bin_y.set(biny);
}

void ArvGeneric::set_bin(int const bin_x, int const bin_y)
{
    this->cam.bin_x.set(bin_x);
    this->cam.bin_y.set(bin_y);

    GError *error = nullptr;
    if (!error) { arv_camera_set_binning(this->camera, this->cam.bin_x.val(), this->cam.bin_y.val(), &error); }
    if (error) { this->indiccd->LogString("Encountered error setting binning!"); g_clear_error(&error); }
}

void ArvGeneric::_test_exposure_and_abort(void)
{
    if (this->_stream_active())
        this->exposure_abort();
}

template <typename T>
void ArvGeneric::_set_cam_exposure_property(void (*arv_set)(::ArvCamera *, T, GError **error), min_max_property<T> *prop,
                                            T const new_val)
{
    this->_test_exposure_and_abort();
    prop->set(new_val);
    GError *error = nullptr;
    if (!error && this->is_connected()) { arv_set(this->camera, prop->val(), &error); }
    if (error) { this->indiccd->LogString("Encountered error !"); g_clear_error(&error); }
    if (!this->is_connected()) 
    {
        this->connect();  
    }
}

void ArvGeneric::set_gain(double const val)
{
    this->_set_cam_exposure_property(arv_camera_set_gain, &this->cam.gain, val);
}
void ArvGeneric::set_exposure_time(double const val)
{
    this->_set_cam_exposure_property(arv_camera_set_exposure_time, &this->cam.exposure, val);
}

::ArvBuffer *ArvGeneric::_buffer_create(void)
{
    ::ArvBuffer *buffer;


    /* Ensure no buffers in stream */
    if (this->stream) {
    	while (1)
    	{
    	    buffer = arv_stream_try_pop_buffer(this->stream);
    	    if (buffer)
    	        g_clear_object(&buffer);
    	    else
    	        break;
    	}
    }

    GError *error = nullptr;

    gint const payload = arv_camera_get_payload(this->camera, &error);
    buffer             = arv_buffer_new(payload, nullptr);
    if (!error) {
        arv_stream_push_buffer(this->stream, buffer);
        return buffer;
    } else {
        this->indiccd->LogString("Encountered error creating new buffer");
        return nullptr;
    }
    g_clear_error(&error);
}

::ArvStream *ArvGeneric::_stream_create(void)
{
    GError *error = nullptr;
    ::ArvStream *stream = arv_camera_create_stream(this->camera, nullptr, nullptr, &error);
    if (!error) {
        g_clear_error(&error);
        return stream;
    } else {
        this->indiccd->LogString("Error creating stream");
        g_clear_error(&error);
        return nullptr;
    }

}

void ArvGeneric::_stream_start()
{
    this->stream_active = true;

    GError *error = nullptr;
    /* Start the acquisition stream */
    arv_camera_set_acquisition_mode(this->camera, ARV_ACQUISITION_MODE_SINGLE_FRAME, &error);
    if (!error) { arv_camera_start_acquisition(this->camera, &error); }
    if (error) { this->indiccd->LogString("Encountered error starting stream!"); g_clear_error(&error); }
}

void ArvGeneric::_stream_stop()
{
    /* stop the acquisition stream */
    this->indiccd->LogString("Stopping the acquisition stream");
    GError *error = nullptr;
    arv_camera_stop_acquisition(this->camera, &error);
    g_object_unref(this->stream);
    if (error) { this->indiccd->LogString("Encountered error unreferencing stream and stopping acquisition!"); g_clear_error(&error); }
    this->stream_active = false;
}

void ArvGeneric::_trigger_exposure()
{
    GError *error = nullptr;
    /* Trigger for an exposure */
    arv_camera_software_trigger(this->camera, &error);
    if (error) { this->indiccd->LogString("Encountered error triggering exposure!"); g_clear_error(&error); }
}

void ArvGeneric::exposure_start(void)
{
    this->_test_exposure_and_abort();
    this->stream = this->_stream_create();
    this->buffer = this->_buffer_create();
    if (this->buffer) // _buffer_create() might return a nullptr on error
    {
        this->_stream_start();
        this->_trigger_exposure();
    }
    else
        throw std::runtime_error("Could not create buffer, maybe camera has been disconnected?");

}

void ArvGeneric::exposure_abort(void)
{
    if (this->_stream_active())
    {
        GError *error = nullptr;
        arv_camera_abort_acquisition(this->camera, &error);
        this->_stream_stop();
        if (error) { this->indiccd->LogString("Encountered error aborting acquisition"); g_clear_error(&error); }
    }
}

void ArvGeneric::_get_image(void (*fn_image_callback)(void *const, uint8_t const *const, size_t), void *const usr_ptr)
{
    ArvBuffer *const popped_buf = arv_stream_timeout_pop_buffer(this->stream, 100000);
    if ((popped_buf != nullptr) && (popped_buf == this->buffer) &&
        arv_buffer_get_status(this->buffer) == ARV_BUFFER_STATUS_SUCCESS)
    {
        if (fn_image_callback != nullptr)
        {
            size_t size;
            const uint8_t* data = (const uint8_t*)arv_buffer_get_data(this->buffer, &size);
            fn_image_callback(usr_ptr, data, size);
        }
    }
    else
    {
	    // TODO: Failure
	    this->indiccd->LogString("Failure in _get_image");
    }
}

ARV_EXPOSURE_STATUS ArvGeneric::exposure_poll(void (*fn_image_callback)(void *const, uint8_t const *const, size_t),
                                              void *const usr_ptr)
{
    if (!this->_stream_active())
        return ARV_EXPOSURE_UNKNOWN;
    
    ::ArvBufferStatus const status = arv_buffer_get_status(this->buffer);
    switch (status)
    {
        case ARV_BUFFER_STATUS_CLEARED:
            return ARV_EXPOSURE_BUSY;
        case ARV_BUFFER_STATUS_FILLING:
            return ARV_EXPOSURE_FILLING;
        case ARV_BUFFER_STATUS_UNKNOWN:
            return ARV_EXPOSURE_UNKNOWN;
        case ARV_BUFFER_STATUS_SUCCESS:
            this->_get_image(fn_image_callback, usr_ptr);
            this->_stream_stop();
            return ARV_EXPOSURE_FINISHED;
        case ARV_BUFFER_STATUS_TIMEOUT:
            this->indiccd->LogString("Encountered timeout error");
            return ARV_EXPOSURE_FAILED;
        case ARV_BUFFER_STATUS_MISSING_PACKETS:
            this->indiccd->LogString("Encountered missing packets error");
            return ARV_EXPOSURE_FAILED;
        case ARV_BUFFER_STATUS_WRONG_PACKET_ID:
            this->indiccd->LogString("Encountered wrong packet ID error");
            return ARV_EXPOSURE_FAILED;
        case ARV_BUFFER_STATUS_SIZE_MISMATCH:
            this->indiccd->LogString("Encountered size mismatch error");
            return ARV_EXPOSURE_FAILED;
        case ARV_BUFFER_STATUS_ABORTED:
            this->_stream_stop();
            return ARV_EXPOSURE_FAILED;
        default:
            return ARV_EXPOSURE_UNKNOWN;
    }
}

double ArvGeneric::get_temperature()
{
    return -150.0;
}

// Reset camera settings (load default)
void ArvGeneric::reset_camera()
{
    GError *error;
    arv_device_set_string_feature_value(this->dev, "UserSetDefaultSelector", "Default", &error);
    arv_device_execute_command(this->dev, "UserSetLoad", &error);

    if (error)
    {
        this->indiccd->LogString("Error trying to reset the camera! damn");
    }
}
