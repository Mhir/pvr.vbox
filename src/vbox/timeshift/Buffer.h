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

#include <string>
#include <ctime>
#include "../../client.h"

namespace vbox {
  namespace timeshift {

    /**
     * The basic type all buffers operate on
     */
#ifndef _WIN32
    typedef unsigned char byte;
#endif

    /**
     * Base class for all timeshift buffers
     */
    class Buffer
    {
    public:

      const static int INPUT_READ_LENGTH = 32768;
      const static int READ_TIMEOUT = 10;

      Buffer() {};
      virtual ~Buffer();

      /**
       * Opens the input handle
       * @return whether the input was successfully opened
       */
      virtual bool Open(const std::string inputUrl);

      /**
       * Reads "length" bytes into the specified buffer
       * @return the number of bytes read
       */
      virtual int Read(byte *buffer, size_t length) = 0;

      /**
       * Seeks to the specified position
       * @return the new position
       */
      virtual int64_t Seek(int64_t position, int whence) = 0;

      /**
       * Whether the buffer supports pausing
       */
      virtual bool CanPauseStream() const = 0;

      /**
       * Whether the buffer supports seeking
       */
      virtual bool CanSeekStream() const = 0;

      /**
       * @return the current read position
       */
      virtual int64_t Position() const = 0;

      /**
       * @return the current length of the buffer
       */
      virtual int64_t Length() const = 0;

      /**
       * @return the time the buffering started
       */
      time_t GetStartTime() const
      {
        return m_startTime;
      }

      /**
       * @return basically the current time
       */
      time_t GetEndTime() const
      {
        return time(nullptr);
      }

    protected:

      /**
       * The input handle (where data is read from)
       */
      void *m_inputHandle;

    private:
      /**
       * The time the buffer was created
       */
      time_t m_startTime;
    };
  }
}
