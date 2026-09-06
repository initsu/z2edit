#ifndef Z2HD_UTIL_STATUS_H
#define Z2HD_UTIL_STATUS_H
#include <string>

#include "absl/status/status.h"

namespace util {

using absl::Status;

namespace error {
using Code = absl::StatusCode;
}

std::string StrError(int error);
Status PosixStatus(int error);

}

#endif // Z2HD_UTIL_STATUS_H