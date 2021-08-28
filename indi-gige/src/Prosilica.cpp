/*
 Custom functions for Prosilica GT cameras
 Copyright (C) 2021 Linus Molteno (linus@molteno.net)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "ArvGeneric.h"
#include "Prosilica.h"

using namespace arv;

min_max_property<int> Prosilica::get_bpp()
{
    return min_max_property<int>(16, 16, 16); // in reality, it's 12, but INDI only does 8,16
}

Prosilica::Prosilica(void *camera_device) : ArvGeneric(camera_device)
{
    this->_custom_settings();
    printf("Finished Prosilica constructor");
}

bool Prosilica::_custom_settings()
{
    ::ArvDevice *dev = this->dev;

    // Features to set: "BalanceWhiteAuto" "Off"
    // 			"SensorDigitizationTaps" "One"
    // 			"SensorTaps" "One" (?)

    GError *error = nullptr;
    arv_device_set_string_feature_value(dev, "SensorDigitizationTaps", "Four", &error);
    if (!error) { arv_device_set_string_feature_value(dev, "BalanceWhiteAuto", "Off", &error); }
    if (!error) { arv_device_set_string_feature_value(dev, "DeviceTemperatureSelector", "Main", &error); }
    if (!error) { arv_camera_set_pixel_format(this->camera, ARV_PIXEL_FORMAT_BAYER_GR_12, &error); }
    if (!error) { arv_camera_set_binning(this->camera, 1, 1, &error); }

    if (error) {
        g_clear_error(&error);
        return false;
    }

    g_clear_error(&error);
    printf("Success!");
    return true;
}

double Prosilica::get_temperature()
{
    GError *error = nullptr;
    double temp = arv_device_get_float_feature_value(this->dev, "DeviceTemperature", &error);
    if (error) {
        g_clear_error(&error);
        return -150.0;
    }
    return temp;
}

bool Prosilica::connect(void)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    bool const ret = ArvGeneric::connect();
    return ret;
}
