#pragma once
/*
*      Copyright (C) 2015 Sam Stenvall
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
*  MA 02110-1301  USA
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#include <memory>
#include <string>
#include "../request/IRequest.h"
#include "Response.h"

namespace vbox {
  namespace response {

    /**
     * Factor for response objects
     */
    class Factory {
    public:

      /**
      * Prevents construction
      */
      Factory() = delete;

      /**
       * Factory method for creating response objects
       * @param request the request
       * @return the response
       */
      static ResponsePtr CreateResponse(const request::IRequest &request)
      {
        switch (request.GetResponseType())
        {
        case ResponseType::XMLTV:
          return ResponsePtr(new XMLTVResponse);
        case ResponseType::RECORDS:
          return ResponsePtr(new RecordingResponse);
        case ResponseType::GENERIC:
        default:
          return ResponsePtr(new Response);
        }
      }
    };
  }
}
