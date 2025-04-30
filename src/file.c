/**
* @file file.c
* @brief File management implementation for libcoil-dev
*/

#include <coil/base.h>
#include "srcdeps.h"
#include <fcntl.h>
#include <unistd.h>

/**
* @brief Open File at descriptor
*/
coil_err_t coil_open_file(coil_descriptor_t *fd, const char *path) {
  if (path == NULL || fd == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Path or descriptor pointer is NULL");
  }
  
  int open_fd = open(path, O_RDWR | O_CREAT, 0644);
  if (open_fd < 0) {
    return COIL_ERROR(COIL_ERR_IO, "Failed to open file");
  }
  
  *fd = open_fd;
  return COIL_ERR_GOOD;
}

/**
* @brief Close descriptor
*/
coil_err_t coil_close(coil_descriptor_t fd) {
  if (fd < 0) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid file descriptor");
  }
  
  if (close(fd) != 0) {
    return COIL_ERROR(COIL_ERR_IO, "Failed to close file descriptor");
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Write to descriptor
*/
coil_err_t coil_write(coil_descriptor_t fd, const coil_byte_t *bytes, 
                     coil_size_t bytecount, coil_size_t *byteswritten) {
  if (fd < 0 || bytes == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid file descriptor or buffer");
  }
  
  ssize_t result = write(fd, bytes, bytecount);
  if (result < 0) {
    return COIL_ERROR(COIL_ERR_IO, "Write operation failed");
  }
  
  if (byteswritten != NULL) {
    *byteswritten = (coil_size_t)result;
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Read from descriptor
*/
coil_err_t coil_read(coil_descriptor_t fd, coil_byte_t *bytes, 
                    coil_size_t bytecount, coil_size_t *bytesread) {
  if (fd < 0 || bytes == NULL) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid file descriptor or buffer");
  }
  
  ssize_t result = read(fd, bytes, bytecount);
  if (result < 0) {
    return COIL_ERROR(COIL_ERR_IO, "Read operation failed");
  }
  
  if (bytesread != NULL) {
    *bytesread = (coil_size_t)result;
  }
  
  return COIL_ERR_GOOD;
}

/**
* @brief Seek descriptor
*/
coil_err_t coil_seek(coil_descriptor_t fd, long int pos, int whence) {
  if (fd < 0) {
    return COIL_ERROR(COIL_ERR_INVAL, "Invalid file descriptor");
  }
  
  if (lseek(fd, pos, whence) == -1) {
    return COIL_ERROR(COIL_ERR_IO, "Seek operation failed");
  }
  
  return COIL_ERR_GOOD;
}