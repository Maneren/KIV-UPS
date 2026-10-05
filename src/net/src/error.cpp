#include <cerrno>
#include <net/error.h>

namespace net::error {

std::string_view to_string(error::ErrorKind kind) {
  switch (kind) {
  case ErrorKind::AddrInUse:
    return "address in use";
  case ErrorKind::AddrNotAvailable:
    return "address not available";
  case ErrorKind::AlreadyExists:
    return "entity already exists";
  case ErrorKind::ArgumentListTooLong:
    return "argument list too long";
  case ErrorKind::BrokenPipe:
    return "broken pipe";
  case ErrorKind::ConnectionAborted:
    return "connection aborted";
  case ErrorKind::ConnectionRefused:
    return "connection refused";
  case ErrorKind::ConnectionReset:
    return "connection reset";
  case ErrorKind::CrossesDevices:
    return "cross-device link or rename";
  case ErrorKind::Deadlock:
    return "deadlock";
  case ErrorKind::DirectoryNotEmpty:
    return "directory not empty";
  case ErrorKind::ExecutableFileBusy:
    return "executable file busy";
  case ErrorKind::FilesystemLoop:
    return "filesystem loop or indirection limit (e.g. symlink loop)";
  case ErrorKind::FileTooLarge:
    return "file too large";
  case ErrorKind::HostUnreachable:
    return "host unreachable";
  case ErrorKind::InProgress:
    return "in progress";
  case ErrorKind::Interrupted:
    return "operation interrupted";
  case ErrorKind::InvalidData:
    return "invalid data";
  case ErrorKind::InvalidFilename:
    return "invalid filename";
  case ErrorKind::InvalidInput:
    return "invalid input parameter";
  case ErrorKind::IsADirectory:
    return "is a directory";
  case ErrorKind::NetworkDown:
    return "network down";
  case ErrorKind::NetworkUnreachable:
    return "network unreachable";
  case ErrorKind::NotADirectory:
    return "not a directory";
  case ErrorKind::NotConnected:
    return "not connected";
  case ErrorKind::NotFound:
    return "entity not found";
  case ErrorKind::NotSeekable:
    return "seek on unseekable file";
  case ErrorKind::Other:
    return "other error";
  case ErrorKind::OutOfMemory:
    return "out of memory";
  case ErrorKind::PermissionDenied:
    return "permission denied";
  case ErrorKind::QuotaExceeded:
    return "quota exceeded";
  case ErrorKind::ReadOnlyFilesystem:
    return "read-only filesystem or storage medium";
  case ErrorKind::ResourceBusy:
    return "resource busy";
  case ErrorKind::StaleNetworkFileHandle:
    return "stale network file handle";
  case ErrorKind::StorageFull:
    return "no storage space";
  case ErrorKind::TimedOut:
    return "timed out";
  case ErrorKind::TooManyLinks:
    return "too many links";
  case ErrorKind::Uncategorized:
    return "uncategorized error";
  case ErrorKind::UnexpectedEof:
    return "unexpected end of file";
  case ErrorKind::Unsupported:
    return "unsupported";
  case ErrorKind::WouldBlock:
    return "operation would block";
  case ErrorKind::WriteZero:
    return "write zero";
  }
}

ErrorKind from_errno(int code) {
  switch (code) {
  case EACCES:
  case EPERM:
    return ErrorKind::PermissionDenied;
  case EADDRINUSE:
    return ErrorKind::AddrInUse;
  case EADDRNOTAVAIL:
    return ErrorKind::AddrNotAvailable;
  case EAGAIN:
#if EWOULDBLOCK != EAGAIN
  case EWOULDBLOCK:
#endif
    return ErrorKind::WouldBlock;
  case EALREADY:
  case EINPROGRESS:
    return ErrorKind::InProgress;
  case EBUSY:
    return ErrorKind::ResourceBusy;
  case ECONNABORTED:
    return ErrorKind::ConnectionAborted;
  case ECONNREFUSED:
    return ErrorKind::ConnectionRefused;
  case ECONNRESET:
    return ErrorKind::ConnectionReset;
  case EDEADLK:
    return ErrorKind::Deadlock;
  case EDQUOT:
    return ErrorKind::QuotaExceeded;
  case EEXIST:
    return ErrorKind::AlreadyExists;
  case EFBIG:
  case EOVERFLOW:
    return ErrorKind::FileTooLarge;
  case EHOSTUNREACH:
    return ErrorKind::HostUnreachable;
  case EINTR:
    return ErrorKind::Interrupted;
  case EINVAL:
    return ErrorKind::InvalidInput;
  case EISDIR:
    return ErrorKind::IsADirectory;
  case ELOOP:
    return ErrorKind::FilesystemLoop;
  case ENOENT:
  case ESRCH:
    return ErrorKind::NotFound;
  case ENOMEM:
    return ErrorKind::OutOfMemory;
  case ENOSPC:
  case ENFILE:
  case EMFILE:
    return ErrorKind::StorageFull;
  case ENOSYS:
  case ENOTTY:
#if defined(EOPNOTSUPP)
  case EOPNOTSUPP:
#endif
#ifdef ENOTSUP
#if ENOTSUP != EOPNOTSUPP
  case ENOTSUP:
#endif
#endif
    return ErrorKind::Unsupported;
  case EMLINK:
    return ErrorKind::TooManyLinks;
  case ENAMETOOLONG:
    return ErrorKind::InvalidFilename;
  case E2BIG:
    return ErrorKind::ArgumentListTooLong;
  case ENETDOWN:
    return ErrorKind::NetworkDown;
  case ENETUNREACH:
    return ErrorKind::NetworkUnreachable;
  case ENOTCONN:
    return ErrorKind::NotConnected;
  case ENOTDIR:
    return ErrorKind::NotADirectory;
  case ENOTEMPTY:
    return ErrorKind::DirectoryNotEmpty;
  case EPIPE:
    return ErrorKind::BrokenPipe;
  case EROFS:
    return ErrorKind::ReadOnlyFilesystem;
  case ESPIPE:
    return ErrorKind::NotSeekable;
  case ESTALE:
    return ErrorKind::StaleNetworkFileHandle;
  case ETIMEDOUT:
    return ErrorKind::TimedOut;
  case ETXTBSY:
    return ErrorKind::ExecutableFileBusy;
  case EXDEV:
    return ErrorKind::CrossesDevices;
  default:
    return ErrorKind::Uncategorized;
  }
}

} // namespace net::error
