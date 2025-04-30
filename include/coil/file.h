/**
* @file file.h
* @brief File management functionality for libcoil-dev
*/

#ifndef __COIL_INCLUDE_GUARD_FILE_H
#define __COIL_INCLUDE_GUARD_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief COIL Descriptor Alias
*/
typedef int coil_descriptor_t;

/**
* @brief Open File at descriptor
*/
coil_err_t coil_open_file(coil_descriptor_t fd, const char *path);

/**
* @brief Close descriptor
*/
coil_err_t coil_close(coil_descriptor_t fd);

/**
* @brief Write to descriptor
*/
coil_err_t coil_write(coil_descriptor_t fd, const coil_byte_t *bytes, coil_size_t bytecount, coil_size_t *byteswritten);

/**
* @brief Read from descriptor
*/
coil_err_t coil_read(coil_descriptor_t fd, coil_byte_t *byte, coil_size_t bytecount, coil_size_t *bytesread);

/**
* @brief Seek descriptor
*/
coil_err_t coil_seek(coil_descriptor_t fd, long int pos, int whence);

#ifdef __cplusplus
}
#endif

#endif // __COIL_INCLUDE_GUARD_FILE_H