/* coil/stream.h */
#ifndef COIL_STREAM_H
#define COIL_STREAM_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "coil/log.h"
#include "coil/err.h"

/* Stream seeking origins */
typedef enum {
    COIL_SEEK_SET = 0,  /* Beginning of stream */
    COIL_SEEK_CUR,      /* Current position */
    COIL_SEEK_END       /* End of stream */
} coil_seek_origin_t;

/* Stream flags */
typedef enum {
    COIL_STREAM_READ  = (1 << 0),  /* Stream is readable */
    COIL_STREAM_WRITE = (1 << 1),  /* Stream is writable */
    COIL_STREAM_SEEK  = (1 << 2),  /* Stream is seekable */
    COIL_STREAM_EOF   = (1 << 3)   /* End of stream has been reached */
} coil_stream_flags_t;

/* Forward declaration */
typedef struct coil_stream coil_stream_t;

/* Stream methods */
typedef size_t (*coil_stream_read_fn)(coil_stream_t *stream, void *buffer, size_t size);
typedef size_t (*coil_stream_write_fn)(coil_stream_t *stream, const void *buffer, size_t size);
typedef int64_t (*coil_stream_seek_fn)(coil_stream_t *stream, int64_t offset, coil_seek_origin_t origin);
typedef int64_t (*coil_stream_tell_fn)(coil_stream_t *stream);
typedef bool (*coil_stream_eof_fn)(coil_stream_t *stream);
typedef void (*coil_stream_close_fn)(coil_stream_t *stream);

/* Stream state */
struct coil_stream {
    void *data;                        /* Stream-specific data */
    coil_stream_read_fn read;          /* Read function */
    coil_stream_write_fn write;        /* Write function */
    coil_stream_seek_fn seek;          /* Seek function */
    coil_stream_tell_fn tell;          /* Tell function */
    coil_stream_eof_fn eof;            /* EOF function */
    coil_stream_close_fn close;        /* Close function */
    uint32_t flags;                    /* Stream flags */
    coil_error_manager_t *error_mgr;   /* Error manager */
    coil_logger_t *logger;             /* Logger */
    pthread_mutex_t lock;              /* Mutex for thread-safety */
    bool initialized;                  /* Whether the stream is initialized */
    const char *name;                  /* Stream name for debugging */
    coil_stream_pos_t position;        /* Current position in the stream */
};

/* Initialize a stream */
int coil_stream_init(coil_stream_t *stream, 
                    const char *name,
                    void *data, 
                    coil_stream_read_fn read,
                    coil_stream_write_fn write,
                    coil_stream_seek_fn seek,
                    coil_stream_tell_fn tell,
                    coil_stream_eof_fn eof,
                    coil_stream_close_fn close,
                    uint32_t flags,
                    coil_error_manager_t *error_mgr,
                    coil_logger_t *logger);

/* Read from a stream */
size_t coil_stream_read(coil_stream_t *stream, void *buffer, size_t size);

/* Write to a stream */
size_t coil_stream_write(coil_stream_t *stream, const void *buffer, size_t size);

/* Seek within a stream */
int64_t coil_stream_seek(coil_stream_t *stream, int64_t offset, coil_seek_origin_t origin);

/* Get current position in a stream */
int64_t coil_stream_tell(coil_stream_t *stream);

/* Check if the end of stream has been reached */
bool coil_stream_eof(coil_stream_t *stream);

/* Close a stream */
void coil_stream_close(coil_stream_t *stream);

/* Get stream current position info */
coil_stream_pos_t coil_stream_get_position(coil_stream_t *stream);

/* Read primitive types */
bool coil_stream_read_uint8(coil_stream_t *stream, uint8_t *value);
bool coil_stream_read_int8(coil_stream_t *stream, int8_t *value);
bool coil_stream_read_uint16(coil_stream_t *stream, uint16_t *value);
bool coil_stream_read_int16(coil_stream_t *stream, int16_t *value);
bool coil_stream_read_uint32(coil_stream_t *stream, uint32_t *value);
bool coil_stream_read_int32(coil_stream_t *stream, int32_t *value);
bool coil_stream_read_uint64(coil_stream_t *stream, uint64_t *value);
bool coil_stream_read_int64(coil_stream_t *stream, int64_t *value);
bool coil_stream_read_float(coil_stream_t *stream, float *value);
bool coil_stream_read_double(coil_stream_t *stream, double *value);

/* Write primitive types */
bool coil_stream_write_uint8(coil_stream_t *stream, uint8_t value);
bool coil_stream_write_int8(coil_stream_t *stream, int8_t value);
bool coil_stream_write_uint16(coil_stream_t *stream, uint16_t value);
bool coil_stream_write_int16(coil_stream_t *stream, int16_t value);
bool coil_stream_write_uint32(coil_stream_t *stream, uint32_t value);
bool coil_stream_write_int32(coil_stream_t *stream, int32_t value);
bool coil_stream_write_uint64(coil_stream_t *stream, uint64_t value);
bool coil_stream_write_int64(coil_stream_t *stream, int64_t value);
bool coil_stream_write_float(coil_stream_t *stream, float value);
bool coil_stream_write_double(coil_stream_t *stream, double value);

/* File stream */
coil_stream_t *coil_file_stream_open(const char *filename, 
                                    const char *mode, 
                                    coil_error_manager_t *error_mgr,
                                    coil_logger_t *logger);

/* Memory stream */
coil_stream_t *coil_memory_stream_create(void *buffer, 
                                        size_t size, 
                                        uint32_t flags,
                                        coil_error_manager_t *error_mgr,
                                        coil_logger_t *logger);

/* Cleanup */
void coil_stream_cleanup(coil_stream_t *stream);

#endif /* COIL_STREAM_H */