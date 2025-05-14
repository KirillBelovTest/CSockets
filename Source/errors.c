#include "WolframLibrary.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <unistd.h>
  #include <errno.h>
#endif

#include "errors.h"

// acceptErrorMessage: maps accept() errors to Wolfram messages
void acceptErrorMessage(WolframLibraryData libData, int err) {
  #ifdef _WIN32
  if (err == WSAEINTR)
  #else
  if (err == EINTR)
  #endif
    libData->Message("acceptRetry");

  #ifdef _WIN32
  else if (err == WSAEWOULDBLOCK)
  #else
  else if (err == EAGAIN || err == EWOULDBLOCK)
  #endif
    libData->Message("acceptDelayRetry");

  #ifdef _WIN32
  else if (err == WSAEMFILE || err == WSAENOBUFS || err == WSAENETDOWN || err == WSAEINVAL)
  #else
  else if (err == EMFILE || err == ENFILE || err == ENOBUFS || err == ENOMEM || err == EOPNOTSUPP)
  #endif
    libData->Message("acceptCloseSocket");

  #ifdef _WIN32
  else if (err == WSAENOTSOCK || err == WSAEINVAL || err == WSAEFAULT)
  #else
  else if (err == EBADF || err == EINVAL || err == EFAULT)
  #endif
    libData->Message("acceptFixParams");

  #ifdef _WIN32
  else if (err == WSAEINVALIDPROVIDER || err == WSAEPROVIDERFAILEDINIT || err == WSASYSCALLFAILURE)
    libData->Message("acceptReinitWinsock");
  #endif

  #ifndef _WIN32
  else if (err == ENOBUFS || err == ENOMEM || err == ENETDOWN
        || err == ENETUNREACH || err == ENETRESET
        || err == EAFNOSUPPORT || err == EPROTONOSUPPORT
        || err == ESOCKTNOSUPPORT)
    libData->Message("acceptRestartProcess");
  #endif

  else
    libData->Message("acceptUnexpectedError");
}

// recvErrorMessage: maps recv() errors to Wolfram messages
void recvErrorMessage(WolframLibraryData libData, int err) {
  if (err == 0) {
    libData->Message("recvGracefulClose");
    return;
  }

  #ifdef _WIN32
  if (err == WSAEINTR)
  #else
  if (err == EINTR)
  #endif
    libData->Message("recvRetry");

  #ifdef _WIN32
  else if (err == WSAEWOULDBLOCK)
  #else
  else if (err == EAGAIN || err == EWOULDBLOCK)
  #endif
    libData->Message("recvDelayRetry");

  #ifdef _WIN32
  else if (err == WSAECONNRESET || err == WSAECONNABORTED || err == WSAESHUTDOWN || err == WSAENOTCONN)
  #else
  else if (err == ECONNRESET || err == ECONNABORTED || err == ESHUTDOWN || err == ENOTCONN)
  #endif
    libData->Message("recvCloseSocket");

  #ifdef _WIN32
  else if (err == WSAENOTSOCK || err == WSAEINVAL || err == WSAEFAULT)
  #else
  else if (err == EBADF || err == EINVAL || err == EFAULT)
  #endif
    libData->Message("recvFixParams");

  #ifdef _WIN32
  else if (err == WSAEINVALIDPROVIDER || err == WSAEPROVIDERFAILEDINIT || err == WSASYSCALLFAILURE)
    libData->Message("recvReinitWinsock");
  #endif

  #ifndef _WIN32
  else if (err == ENOBUFS || err == ENOMEM || err == ENETDOWN
        || err == ENETUNREACH || err == ENETRESET
        || err == EAFNOSUPPORT || err == EPROTONOSUPPORT
        || err == ESOCKTNOSUPPORT)
    libData->Message("recvRestartProcess");
  #endif

  else
    libData->Message("recvUnexpectedError");
}

// selectErrorMessage: maps select() errors to Wolfram messages
void selectErrorMessage(WolframLibraryData libData, int err) {
  #ifdef _WIN32
  if (err == WSAEINTR)
  #else
  if (err == EINTR)
  #endif
    libData->Message("selectRetry");

  #ifdef _WIN32
  else if (err == WSAENOTSOCK || err == WSAEFAULT || err == WSAEINVAL || err == WSAENOBUFS)
  #else
  else if (err == EBADF || err == EFAULT || err == EINVAL || err == ENOMEM)
  #endif
    libData->Message("selectClose");

  #ifdef _WIN32
  else if (err == WSAENOTSOCK || err == WSAEINVAL || err == WSAEFAULT)
  #else
  else if (err == EBADF || err == EINVAL || err == EFAULT)
  #endif
    libData->Message("selectFixParams");

  #ifdef _WIN32
  else if (err == WSAEINVALIDPROVIDER || err == WSAEPROVIDERFAILEDINIT || err == WSASYSCALLFAILURE)
    libData->Message("selectReinitWinsock");
  #endif

  #ifndef _WIN32
  else if (err == ENOBUFS || err == ENOMEM || err == ENETDOWN
        || err == ENETUNREACH || err == ENETRESET
        || err == EAFNOSUPPORT || err == EPROTONOSUPPORT
        || err == ESOCKTNOSUPPORT)
    libData->Message("selectRestartProcess");
  #endif

  else
    libData->Message("selectUnexpectedError");
}

// sendErrorMessage: maps send() errors to Wolfram messages
void sendErrorMessage(WolframLibraryData libData, int err) {
  #ifdef _WIN32
  if (err == WSAEINTR)
  #else
  if (err == EINTR)
  #endif
    libData->Message("sendRetry");

  #ifdef _WIN32
  else if (err == WSAEWOULDBLOCK)
  #else
  else if (err == EAGAIN || err == EWOULDBLOCK)
  #endif
    libData->Message("sendDelayRetry");

  #ifdef _WIN32
  else if (err == WSAENOTCONN || err == WSAESHUTDOWN || err == WSAECONNRESET)
  #else
  else if (err == ENOTCONN || err == EPIPE || err == ECONNRESET || err == ESHUTDOWN)
  #endif
    libData->Message("sendCloseSocket");

  #ifdef _WIN32
  else if (err == WSAEMSGSIZE)
  #else
  else if (err == EMSGSIZE)
  #endif
    libData->Message("sendMsgSize");

  #ifdef _WIN32
  else if (err == WSAENOTSOCK || err == WSAEINVAL || err == WSAEFAULT)
  #else
  else if (err == EBADF || err == EINVAL || err == EFAULT)
  #endif
    libData->Message("sendFixParams");

  #ifdef _WIN32
  else if (err == WSAEINVALIDPROVIDER || err == WSAEPROVIDERFAILEDINIT || err == WSASYSCALLFAILURE)
    libData->Message("sendReinitWinsock");
  #endif

  #ifndef _WIN32
  else if (err == ENOBUFS || err == ENOMEM || err == ENETDOWN
        || err == ENETUNREACH || err == ENETRESET
        || err == EAFNOSUPPORT || err == EPROTONOSUPPORT
        || err == ESOCKTNOSUPPORT)
    libData->Message("sendRestartProcess");
  #endif

  else
    libData->Message("sendUnexpectedError");
}