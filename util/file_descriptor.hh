#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

// A reference-counted handle to a file descriptor
class FileDescriptor
{
  // FDWrapper: A handle on a kernel file descriptor.
  // FileDescriptor objects contain a std::shared_ptr to a FDWrapper.
  class FDWrapper
  {
  public:
    int fd_;                    // The file descriptor number returned by the kernel
    bool eof_ = false;          // Flag indicating whether FDWrapper::fd_ is at EOF
    bool closed_ = false;       // Flag indicating whether FDWrapper::fd_ has been closed
    bool non_blocking_ = false; // Flag indicating whether FDWrapper::fd_ is non-blocking
    unsigned read_count_ = 0;   // The number of times FDWrapper::fd_ has been read
    unsigned write_count_ = 0;  // The numberof times FDWrapper::fd_ has been written

    // Construct from a file descriptor number returned by the kernel
    explicit FDWrapper( int fd );
    // Closes the file descriptor upon destruction
    ~FDWrapper();
    // Calls [close(2)](\ref man2::close) on FDWrapper::fd_
    void close();

    template<typename T>
    T CheckSystemCall( std::string_view s_attempt, T return_value ) const;

    // An FDWrapper cannot be copied or moved
    FDWrapper( const FDWrapper& other ) = delete;
    FDWrapper& operator=( const FDWrapper& other ) = delete;
    FDWrapper( FDWrapper&& other ) = delete;
    FDWrapper& operator=( FDWrapper&& other ) = delete;
  };

  // A reference-counted handle to a shared FDWrapper
  std::shared_ptr<FDWrapper> internal_fd_;

  // private constructor used to duplicate the FileDescriptor (increase the reference count)
  explicit FileDescriptor( std::shared_ptr<FDWrapper> other_shared_ptr );

protected:
  // size of buffer to allocate for read()
  static constexpr size_t kReadBufferSize = 16384;

  void set_eof() { internal_fd_->eof_ = true; }
  void register_read() { ++internal_fd_->read_count_; }   // increment read count
  void register_write() { ++internal_fd_->write_count_; } // increment write count

  template<typename T>
  T CheckSystemCall( std::string_view s_attempt, T return_value ) const;

public:
  // Construct from a file descriptor number returned by the kernel
  explicit FileDescriptor( int fd );

  // Free the std::shared_ptr; the FDWrapper destructor calls close() when the refcount goes to zero.
  ~FileDescriptor() = default;

  // Read into `buffer`
  void read( std::string& buffer );
  void read( std::vector<std::string>& buffers );

  // Attempt to write a buffer
  // returns number of bytes written
  size_t write( std::string_view buffer );
  size_t write( const std::vector<std::string_view>& buffers );
  size_t write( const std::vector<std::string>& buffers );

  // Close the underlying file descriptor
  void close() { internal_fd_->close(); }

  // Copy a FileDescriptor explicitly, increasing the FDWrapper refcount
  FileDescriptor duplicate() const;

  // Set blocking(true) or non-blocking(false)
  void set_blocking( bool blocking );

  // Size of file
  off_t size() const;

  // FDWrapper accessors
  int fd_num() const { return internal_fd_->fd_; }                        // underlying descriptor number
  bool eof() const { return internal_fd_->eof_; }                         // EOF flag state
  bool closed() const { return internal_fd_->closed_; }                   // closed flag state
  unsigned int read_count() const { return internal_fd_->read_count_; }   // number of reads
  unsigned int write_count() const { return internal_fd_->write_count_; } // number of writes

  // Copy/move constructor/assignment operators
  // FileDescriptor can be moved, but cannot be copied implicitly (see duplicate())
  FileDescriptor( const FileDescriptor& other ) = delete;            // copy construction is forbidden
  FileDescriptor& operator=( const FileDescriptor& other ) = delete; // copy assignment is forbidden
  FileDescriptor( FileDescriptor&& other ) = default;                // move construction is allowed
  FileDescriptor& operator=( FileDescriptor&& other ) = default;     // move assignment is allowed
};
