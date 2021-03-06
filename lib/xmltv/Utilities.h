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

#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

namespace xmltv {
  class Utilities
  {
  public:

    /**
      * The XMLTV datetime format string
      */
    static const char* XMLTV_DATETIME_FORMAT;

    /**
      * Converts an XMLTV datetime string to time_t
      * @param time e.g. "20120228001500+0200"
      * @return a UNIX timestamp
      */
    static time_t XmltvToUnixTime(const std::string &time)
    {
      std::tm timeinfo;

      sscanf(time.c_str(), "%04d%02d%02d%02d%02d%02d",
        &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday, 
        &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);

      timeinfo.tm_year -= 1900;
      timeinfo.tm_mon -= 1;
      timeinfo.tm_isdst = -1;
        
      return mktime(&timeinfo);
    }

    /**
      * Converts a time_t to a GMT XMLTV datetime string
      * @param time a UNIX timestamp
      * @return e.g. "20120228001500+0000"
      */
    static std::string UnixTimeToXmltv(const time_t timestamp)
    {
      std::tm tm = *std::gmtime(&timestamp);
        
      char buffer[20];
      strftime(buffer, sizeof(buffer), XMLTV_DATETIME_FORMAT, &tm);

      std::string xmltvTime(buffer);
      xmltvTime += "+0000";

      return xmltvTime;
    }

    /**
     * URL-encodes the specified string
     *
     * @param name the string to encode
     * @return the encoded string
     */
    static std::string UrlEncode(const std::string &string);

    /**
      * Decodes the specified URL
      *
      * @param name the string to decode
      * @return the decoded string
      */
    static std::string UrlDecode(const std::string &string);

    /**
     * Concatenates the contents of vector with the specified separator
     * @param vector the vector
     * @param separator the separator
     * @return a concatenated string
     */
    static std::string ConcatenateStringList(const std::vector<std::string> &vector, 
      const std::string &separator = ", ");
  };
}
