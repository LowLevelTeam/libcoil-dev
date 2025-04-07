/* src/stream.c */
#include "coil/stream.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
                  coil_logger_t *logger) {
                  
  if (!stream) return -1;
  
  stream->data = data;
  stream->read = read;
  stream->write = write;
  stream->seek = seek;
  stream->tell = tell;
  stream->eof = eof;
  stream->close = close;
  stream->flags = flags;
  stream->error_mgr = error_mgr ? error_mgr : coil_default_error_manager;
  stream->logger = logger ? logger : coil_default_logger;
  stream->name = name ? name : "unnamed";
  
  /* Initialize position */
  stream->position.file_name = stream->name;
  stream->position.line = 1;
  stream->position.column = 1;
  stream->position.offset = 0;
  
  if (pthread_mutex_init(&stream->lock, NULL) != 0) {
      return -1;
  }
  
  stream->initialized = true;
  return 0;
}

size_t coil_stream_read(coil_stream_t *stream, void *buffer, size_t size) {
  if (!stream || !stream->initialized || !stream->read || !(stream->flags & COIL_STREAM_READ)) {
      return 0;
  }
  
  pthread_mutex_lock(&stream->lock);
  size_t bytes_read = stream->read(stream, buffer, size);
  
  /* Update position */
  stream->position.offset += bytes_read;
  
  /* If we're tracking lines and columns, we need to scan for newlines */
  if (bytes_read > 0) {
      char *buf = (char *)buffer;
      for (size_t i = 0; i < bytes_read; i++) {
          if (buf[i] == '\n') {
              stream->position.line++;
              stream->position.column = 1;
          } else {
              stream->position.column++;
          }
      }
  }
  
  pthread_mutex_unlock(&stream->lock);
  return bytes_read;
}

size_t coil_stream_write(coil_stream_t *stream, const void *buffer, size_t size) {
  if (!stream || !stream->initialized || !stream->write || !(stream->flags & COIL_STREAM_WRITE)) {
      return 0;
  }
  
  pthread_mutex_lock(&stream->lock);
  size_t bytes_written = stream->write(stream, buffer, size);
  
  /* Update position */
  stream->position.offset += bytes_written;
  
  /* If we're tracking lines and columns, we need to scan for newlines */
  if (bytes_written > 0) {
      const char *buf = (const char *)buffer;
      for (size_t i = 0; i < bytes_written; i++) {
          if (buf[i] == '\n') {
              stream->position.line++;
              stream->position.column = 1;
          } else {
              stream->position.column++;
          }
      }
  }
  
  pthread_mutex_unlock(&stream->lock);
  return bytes_written;
}

int64_t coil_stream_seek(coil_stream_t *stream, int64_t offset, coil_seek_origin_t origin) {
  if (!stream || !stream->initialized || !stream->seek || !(stream->flags & COIL_STREAM_SEEK)) {
      return -1;
  }
  
  pthread_mutex_lock(&stream->lock);
  int64_t result = stream->seek(stream, offset, origin);
  
  /* Update position for byte offset, but we can't reliably track lines and columns after a seek */
  /* So we just reset them to 1 */
  if (result >= 0) {
      stream->position.offset = (size_t)result;
      stream->position.line = 1;
      stream->position.column = 1;
  }
  
  pthread_mutex_unlock(&stream->lock);
  return result;
}

int64_t coil_stream_tell(coil_stream_t *stream) {
  if (!stream || !stream->initialized || !stream->tell) {
      return -1;
  }
  
  pthread_mutex_lock(&stream->lock);
  int64_t result = stream->tell(stream);
  pthread_mutex_unlock(&stream->lock);
  return result;
}

bool coil_stream_eof(coil_stream_t *stream) {
  if (!stream || !stream->initialized || !stream->eof) {
      return true;
  }
  
  pthread_mutex_lock(&stream->lock);
  bool result = stream->eof(stream);
  pthread_mutex_unlock(&stream->lock);
  return result;
}

void coil_stream_close(coil_stream_t *stream) {
  if (!stream || !stream->initialized || !stream->close) {
      return;
  }
  
  pthread_mutex_lock(&stream->lock);
  stream->close(stream);
  pthread_mutex_unlock(&stream->lock);
}

coil_stream_pos_t coil_stream_get_position(coil_stream_t *stream) {
  coil_stream_pos_t pos = {0};
  
  if (!stream || !stream->initialized) {
      return pos;
  }
  
  pthread_mutex_lock(&stream->lock);
  pos = stream->position;
  pthread_mutex_unlock(&stream->lock);
  
  return pos;
}

/* Read primitive types */
bool coil_stream_read_uint8(coil_stream_t *stream, uint8_t *value) {
  if (!stream || !value) return false;
  return coil_stream_read(stream, value, sizeof(uint8_t)) == sizeof(uint8_t);
}

bool coil_stream_read_int8(coil_stream_t *stream, int8_t *value) {
  if (!stream || !value) return false;
  return coil_stream_read(stream, value, sizeof(int8_t)) == sizeof(int8_t);
}

bool coil_stream_read_uint16(coil_stream_t *stream, uint16_t *value) {
  if (!stream || !value) return false;
  return coil_stream_read(stream, value, sizeof(uint16_t)) == sizeof(uint16_t);
}

bool coil_stream_read_int16(coil_stream_t *stream, int16_t *value) {
  if (!stream || !value) return false;
  return coil_stream_read(stream, value, sizeof(int16_t)) == sizeof(int16_t);
}

bool coil_stream_read_uint32(coil_stream_t *stream, uint32_t *value) {
  if (!stream || !value) return false;
  return coil_stream_read(stream, value, sizeof(uint32_t)) == sizeof(uint32_t);
}

bool coil_stream_read_int32(coil_stream_t *stream, int32_t *value) {
  if (!stream || !value) return false;
  return coil_stream_read(stream, value, sizeof(int32_t)) == sizeof(int32_t);
}

bool coil_stream_read_uint64(coil_stream_t *stream, uint64_t *value) {
  if (!stream || !value) return false;
  return coil_stream_read(stream, value, sizeof(uint64_t)) == sizeof(uint64_t);
}

bool coil_stream_read_int64(coil_stream_t *stream, int64_t *value) {
  if (!stream || !value) return false;
  return coil_stream_read(stream, value, sizeof(int64_t)) == sizeof(int64_t);
}

bool coil_stream_read_float(coil_stream_t *stream, float *value) {
  if (!stream || !value) return false;
  return coil_stream_read(stream, value, sizeof(float)) == sizeof(float);
}

bool coil_stream_read_double(coil_stream_t *stream, double *value) {
  if (!stream || !value) return false;
  return coil_stream_read(stream, value, sizeof(double)) == sizeof(double);
}

/* Write primitive types */
bool coil_stream_write_uint8(coil_stream_t *stream, uint8_t value) {
  if (!stream) return false;
  return coil_stream_write(stream, &value, sizeof(uint8_t)) == sizeof(uint8_t);
}

bool coil_stream_write_int8(coil_stream_t *stream, int8_t value) {
  if (!stream) return false;
  return coil_stream_write(stream, &value, sizeof(int8_t)) == sizeof(int8_t);
}

bool coil_stream_write_uint16(coil_stream_t *stream, uint16_t value) {
  if (!stream) return false;
  return coil_stream_write(stream, &value, sizeof(uint16_t)) == sizeof(uint16_t);
}

bool coil_stream_write_int16(coil_stream_t *stream, int16_t value) {
  if (!stream) return false;
  return coil_stream_write(stream, &value, sizeof(int16_t)) == sizeof(int16_t);
}

bool coil_stream_write_uint32(coil_stream_t *stream, uint32_t value) {
  if (!stream) return false;
  return coil_stream_write(stream, &value, sizeof(uint32_t)) == sizeof(uint32_t);
}

bool coil_stream_write_int32(coil_stream_t *stream, int32_t value) {
  if (!stream) return false;
  return coil_stream_write(stream, &value, sizeof(int32_t)) == sizeof(int32_t);
}

bool coil_stream_write_uint64(coil_stream_t *stream, uint64_t value) {
  if (!stream) return false;
  return coil_stream_write(stream, &value, sizeof(uint64_t)) == sizeof(uint64_t);
}

bool coil_stream_write_int64(coil_stream_t *stream, int64_t value) {
  if (!stream) return false;
  return coil_stream_write(stream, &value, sizeof(int64_t)) == sizeof(int64_t);
}

bool coil_stream_write_float(coil_stream_t *stream, float value) {
  if (!stream) return false;
  return coil_stream_write(stream, &value, sizeof(float)) == sizeof(float);
}

bool coil_stream_write_double(coil_stream_t *stream, double value) {
  if (!stream) return false;
  return coil_stream_write(stream, &value, sizeof(double)) == sizeof(double);
}

void coil_stream_cleanup(coil_stream_t *stream) {
  if (!stream || !stream->initialized) return;
  
  if (stream->close) {
      stream->close(stream);
  }
  
  pthread_mutex_destroy(&stream->lock);
  stream->initialized = false;
}

/* File stream implementation */
typedef struct {
  FILE *fp;
} coil_file_stream_data_t;

static size_t coil_file_stream_read(coil_stream_t *stream, void *buffer, size_t size) {
  coil_file_stream_data_t *data = (coil_file_stream_data_t *)stream->data;
  if (!data || !data->fp) return 0;
  
  size_t bytes_read = fread(buffer, 1, size, data->fp);
  
  if (bytes_read < size && ferror(data->fp)) {
      if (stream->error_mgr) {
          coil_error_error(stream->error_mgr, COIL_ERR_IO, &stream->position, 
                          "Error reading from file stream: %s", strerror(ferror(data->fp)));
      }
  }
  
  if (feof(data->fp)) {
      stream->flags |= COIL_STREAM_EOF;
  }
  
  return bytes_read;
}

static size_t coil_file_stream_write(coil_stream_t *stream, const void *buffer, size_t size) {
  coil_file_stream_data_t *data = (coil_file_stream_data_t *)stream->data;
  if (!data || !data->fp) return 0;
  
  size_t bytes_written = fwrite(buffer, 1, size, data->fp);
  
  if (bytes_written < size) {
      if (stream->error_mgr) {
          coil_error_error(stream->error_mgr, COIL_ERR_IO, &stream->position, 
                          "Error writing to file stream: %s", strerror(ferror(data->fp)));
      }
  }
  
  return bytes_written;
}

static int64_t coil_file_stream_seek(coil_stream_t *stream, int64_t offset, coil_seek_origin_t origin) {
  coil_file_stream_data_t *data = (coil_file_stream_data_t *)stream->data;
  if (!data || !data->fp) return -1;
  
  int whence;
  
  switch (origin) {
      case COIL_SEEK_SET: whence = SEEK_SET; break;
      case COIL_SEEK_CUR: whence = SEEK_CUR; break;
      case COIL_SEEK_END: whence = SEEK_END; break;
      default: return -1;
  }
  
  if (fseek(data->fp, (long)offset, whence) != 0) {
      if (stream->error_mgr) {
          coil_error_error(stream->error_mgr, COIL_ERR_IO, &stream->position, 
                          "Error seeking in file stream: %s", strerror(ferror(data->fp)));
      }
      return -1;
  }
  
  /* Clear EOF flag if it was set */
  stream->flags &= ~COIL_STREAM_EOF;
  
  return ftell(data->fp);
}

static int64_t coil_file_stream_tell(coil_stream_t *stream) {
  coil_file_stream_data_t *data = (coil_file_stream_data_t *)stream->data;
  if (!data || !data->fp) return -1;
  
  long pos = ftell(data->fp);
  
  if (pos < 0) {
      if (stream->error_mgr) {
          coil_error_error(stream->error_mgr, COIL_ERR_IO, &stream->position, 
                          "Error getting position in file stream: %s", strerror(ferror(data->fp)));
      }
  }
  
  return pos;
}

static bool coil_file_stream_eof(coil_stream_t *stream) {
  coil_file_stream_data_t *data = (coil_file_stream_data_t *)stream->data;
  if (!data || !data->fp) return true;
  
  return feof(data->fp) != 0 || (stream->flags & COIL_STREAM_EOF);
}

static void coil_file_stream_close(coil_stream_t *stream) {
  coil_file_stream_data_t *data = (coil_file_stream_data_t *)stream->data;
  if (!data || !data->fp) return;
  
  fclose(data->fp);
  data->fp = NULL;
  free(data);
  stream->data = NULL;
}

coil_stream_t *coil_file_stream_open(const char *filename, 
                                  const char *mode, 
                                  coil_error_manager_t *error_mgr,
                                  coil_logger_t *logger) {
  if (!filename || !mode) return NULL;
  
  FILE *fp = fopen(filename, mode);
  if (!fp) {
      if (error_mgr) {
          coil_stream_pos_t pos = coil_stream_pos_create(filename, 0, 0, 0);
          coil_error_error(error_mgr, COIL_ERR_IO, &pos, 
                          "Failed to open file '%s' with mode '%s': %s", 
                          filename, mode, strerror(errno));
      }
      return NULL;
  }
  
  coil_file_stream_data_t *data = (coil_file_stream_data_t *)malloc(sizeof(coil_file_stream_data_t));
  if (!data) {
      fclose(fp);
      
      if (error_mgr) {
          coil_stream_pos_t pos = coil_stream_pos_create(filename, 0, 0, 0);
          coil_error_error(error_mgr, COIL_ERR_MEMORY, &pos, 
                          "Failed to allocate memory for file stream");
      }
      
      return NULL;
  }
  
  data->fp = fp;
  
  coil_stream_t *stream = (coil_stream_t *)malloc(sizeof(coil_stream_t));
  if (!stream) {
      free(data);
      fclose(fp);
      
      if (error_mgr) {
          coil_stream_pos_t pos = coil_stream_pos_create(filename, 0, 0, 0);
          coil_error_error(error_mgr, COIL_ERR_MEMORY, &pos, 
                          "Failed to allocate memory for stream");
      }
      
      return NULL;
  }
  
  uint32_t flags = COIL_STREAM_SEEK;
  
  if (strchr(mode, 'r')) {
      flags |= COIL_STREAM_READ;
  }
  
  if (strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+')) {
      flags |= COIL_STREAM_WRITE;
  }
  
  if (coil_stream_init(stream, 
                      filename,
                      data, 
                      coil_file_stream_read,
                      coil_file_stream_write,
                      coil_file_stream_seek,
                      coil_file_stream_tell,
                      coil_file_stream_eof,
                      coil_file_stream_close,
                      flags,
                      error_mgr,
                      logger) != 0) {
      free(stream);
      free(data);
      fclose(fp);
      
      if (error_mgr) {
          coil_stream_pos_t pos = coil_stream_pos_create(filename, 0, 0, 0);
          coil_error_error(error_mgr, COIL_ERR_INTERNAL, &pos, 
                          "Failed to initialize stream");
      }
      
      return NULL;
  }
  
  return stream;
}

/* Memory stream implementation */
typedef struct {
  uint8_t *buffer;
  size_t size;
  size_t position;
  bool owns_buffer;
} coil_memory_stream_data_t;

static size_t coil_memory_stream_read(coil_stream_t *stream, void *buffer, size_t size) {
  coil_memory_stream_data_t *data = (coil_memory_stream_data_t *)stream->data;
  if (!data || !data->buffer) return 0;
  
  size_t available = data->size - data->position;
  size_t bytes_to_read = size < available ? size : available;
  
  if (bytes_to_read == 0) {
      stream->flags |= COIL_STREAM_EOF;
      return 0;
  }
  
  memcpy(buffer, data->buffer + data->position, bytes_to_read);
  data->position += bytes_to_read;
  
  if (data->position >= data->size) {
      stream->flags |= COIL_STREAM_EOF;
  }
  
  return bytes_to_read;
}

static size_t coil_memory_stream_write(coil_stream_t *stream, const void *buffer, size_t size) {
  coil_memory_stream_data_t *data = (coil_memory_stream_data_t *)stream->data;
  if (!data || !data->buffer) return 0;
  
  size_t available = data->size - data->position;
  size_t bytes_to_write = size < available ? size : available;
  
  if (bytes_to_write == 0) {
      return 0;
  }
  
  memcpy(data->buffer + data->position, buffer, bytes_to_write);
  data->position += bytes_to_write;
  
  return bytes_to_write;
}

static int64_t coil_memory_stream_seek(coil_stream_t *stream, int64_t offset, coil_seek_origin_t origin) {
  coil_memory_stream_data_t *data = (coil_memory_stream_data_t *)stream->data;
  if (!data || !data->buffer) return -1;
  
  int64_t new_position = 0;
  
  switch (origin) {
      case COIL_SEEK_SET:
          new_position = offset;
          break;
      case COIL_SEEK_CUR:
          new_position = (int64_t)data->position + offset;
          break;
      case COIL_SEEK_END:
          new_position = (int64_t)data->size + offset;
          break;
      default:
          return -1;
  }
  
  if (new_position < 0 || new_position > (int64_t)data->size) {
      if (stream->error_mgr) {
          coil_error_error(stream->error_mgr, COIL_ERR_BOUNDS, &stream->position, 
                          "Seek position out of bounds");
      }
      return -1;
  }
  
  data->position = (size_t)new_position;
  
  /* Clear EOF flag if it was set */
  stream->flags &= ~COIL_STREAM_EOF;
  
  return new_position;
}

static int64_t coil_memory_stream_tell(coil_stream_t *stream) {
  coil_memory_stream_data_t *data = (coil_memory_stream_data_t *)stream->data;
  if (!data || !data->buffer) return -1;
  
  return (int64_t)data->position;
}

static bool coil_memory_stream_eof(coil_stream_t *stream) {
  coil_memory_stream_data_t *data = (coil_memory_stream_data_t *)stream->data;
  if (!data || !data->buffer) return true;
  
  return data->position >= data->size || (stream->flags & COIL_STREAM_EOF);
}

static void coil_memory_stream_close(coil_stream_t *stream) {
  coil_memory_stream_data_t *data = (coil_memory_stream_data_t *)stream->data;
  if (!data) return;
  
  if (data->owns_buffer && data->buffer) {
      free(data->buffer);
  }
  
  free(data);
  stream->data = NULL;
}

coil_stream_t *coil_memory_stream_create(void *buffer, 
                                      size_t size, 
                                      uint32_t flags,
                                      coil_error_manager_t *error_mgr,
                                      coil_logger_t *logger) {
  if (!buffer && size > 0) {
      /* Allocate our own buffer */
      buffer = malloc(size);
      if (!buffer) {
          if (error_mgr) {
              coil_stream_pos_t pos = coil_stream_pos_create("memory", 0, 0, 0);
              coil_error_error(error_mgr, COIL_ERR_MEMORY, &pos, 
                              "Failed to allocate memory for memory stream");
          }
          return NULL;
      }
  }
  
  coil_memory_stream_data_t *data = (coil_memory_stream_data_t *)malloc(sizeof(coil_memory_stream_data_t));
  if (!data) {
      if (buffer && size > 0) {
          free(buffer);
      }
      
      if (error_mgr) {
          coil_stream_pos_t pos = coil_stream_pos_create("memory", 0, 0, 0);
          coil_error_error(error_mgr, COIL_ERR_MEMORY, &pos, 
                          "Failed to allocate memory for memory stream data");
      }
      
      return NULL;
  }
  
  data->buffer = buffer;
  data->size = size;
  data->position = 0;
  data->owns_buffer = buffer && size > 0;
  
  coil_stream_t *stream = (coil_stream_t *)malloc(sizeof(coil_stream_t));
  if (!stream) {
      if (data->owns_buffer && data->buffer) {
          free(data->buffer);
      }
      free(data);
      
      if (error_mgr) {
          coil_stream_pos_t pos = coil_stream_pos_create("memory", 0, 0, 0);
          coil_error_error(error_mgr, COIL_ERR_MEMORY, &pos, 
                          "Failed to allocate memory for stream");
      }
      
      return NULL;
  }
  
  /* Always add SEEK for memory streams */
  flags |= COIL_STREAM_SEEK;
  
  if (coil_stream_init(stream, 
                      "memory",
                      data, 
                      coil_memory_stream_read,
                      coil_memory_stream_write,
                      coil_memory_stream_seek,
                      coil_memory_stream_tell,
                      coil_memory_stream_eof,
                      coil_memory_stream_close,
                      flags,
                      error_mgr,
                      logger) != 0) {
      free(stream);
      if (data->owns_buffer && data->buffer) {
          free(data->buffer);
      }
      free(data);
      
      if (error_mgr) {
          coil_stream_pos_t pos = coil_stream_pos_create("memory", 0, 0, 0);
          coil_error_error(error_mgr, COIL_ERR_INTERNAL, &pos, 
                          "Failed to initialize stream");
      }
      
      return NULL;
  }
  
  return stream;
}