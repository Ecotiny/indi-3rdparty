/*
 GigE interface wrapper on araviss
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
#ifndef CPP_ARV_PROSILICA_H
#define CPP_ARV_PROSILICA_H

class Prosilica : public ArvGeneric
{
  public:
    Prosilica(void *camera_device);
    bool connect();
    min_max_property<int> get_bpp();
    double get_temperature();

  private:
    bool _custom_settings();
};

#endif
