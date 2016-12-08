/* <base/error_utils.cc>

   ----------------------------------------------------------------------------
   Copyright 2010-2013 if(we)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   ----------------------------------------------------------------------------

   Implements <base/error_utils.h>.
 */

#include <base/error_utils.h>

#include <cassert>
#include <cstring>

namespace Base {

  const char *Strerror(int errno_value, char *buf, size_t buf_size) {
    assert(buf);
    assert(buf_size);

// Running into issues building against musl libc with the preprocessor here. Falling into
// the GNU-specific variant of the reentrant strerror_r function while the XSI-compliant
// version is what's actually supported. Testing against the return value of strerror_r 
// at run/compile(?) time may be safer than relying on macros to determine which version is 
// supported? Not aware of any downsides.
//
// xref:
//   * https://github.com/chriskohlhoff/asio/issues/94
//   * http://zverovich.net/2015/03/13/reliable-detection-of-strerror-variants.html
//
//    /* The man page for strerror_r explains all of this ugliness. */
//
//#if ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE)
//
//    // Here's one rabbit hole: https://github.com/chriskohlhoff/asio/issues/94
//
//    /* This is the XSI-compliant version of strerror_r(). */
//    int err = strerror_r(errno_value, buf, buf_size);
//
//    if (err) {
//      /* In the unlikely event that something went wrong, make the buffer
//         contain the empty string, in case it would otherwise be left with
//         arbitrary junk. */
//      buf[0] = '\0';
//    }
//
//    return buf;
//#else
//    /* This is the GNU-specific version of strerror_r().  Its return type is
//       'char *'. */
//    return strerror_r(errno_value, buf, buf_size);
//#endif

    return strerror_r_inner_wrapper(strerror_r(errno_value, buf, buf_size), buf);
  }

  const char *strerror_r_inner_wrapper(int ret_value, char *buf) {
    /* Wrapper for XSI-compliant version of strerror_r(). */

    if (ret_value /* err */) {
      /* In the unlikely event that something went wrong, make the buffer
         contain the empty string, in case it would otherwise be left with
         arbitrary junk. */
      buf[0] = '\0';
    }

    return buf;
  }

  const char *strerror_r_inner_wrapper(char *ret_value, char *ignored) {
    /* Wrapper for GNU-specific version of strerror_r() Its return type is
       'char *'. */

    /* Icky hack to get around unused-parameter errors :( */
    if (ignored) {
        return ret_value;
    }

    return ret_value;
  }

}  // Base
