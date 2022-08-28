// Copyright 2021 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

// This file defines the memory file class of the new file system.
// This should be the only backend file type defined in a header since it is the
// default type. Current Status: Work in Progress. See
// https://github.com/emscripten-core/emscripten/issues/15041.

#pragma once

#include "backend.h"
#include "file.h"
#include <emscripten/threading.h>

namespace wasmfs {
// This class describes a file that lives in Wasm Memory.
class MemoryFile : public DataFile {
  std::vector<uint8_t> buffer;

  void open(oflags_t) override {}
  void close() override {}
  ssize_t write(const uint8_t* buf, size_t len, off_t offset) override;
  ssize_t read(uint8_t* buf, size_t len, off_t offset) override;
  void flush() override {}
  size_t getSize() override { return buffer.size(); }
  void setSize(size_t size) override { return buffer.resize(size); }

public:
  MemoryFile(mode_t mode, backend_t backend) : DataFile(mode, backend) {}

  class Handle : public DataFile::Handle {

    std::shared_ptr<MemoryFile> getFile() { return file->cast<MemoryFile>(); }

  public:
    Handle(std::shared_ptr<File> dataFile) : DataFile::Handle(dataFile) {}
  };

  Handle locked() { return Handle(shared_from_this()); }
};

class MemoryDirectory : public Directory {
  // Use a vector instead of a map to save code size.
  struct ChildEntry {
    std::string name;
    std::shared_ptr<File> child;
  };

  std::vector<ChildEntry> entries;

  std::vector<ChildEntry>::iterator findEntry(const std::string& name);

protected:
  void insertChild(const std::string& name, std::shared_ptr<File> child) {
    assert(findEntry(name) == entries.end());
    entries.push_back({name, child});
  }

  std::shared_ptr<File> getChild(const std::string& name) override {
    return nullptr;
  }

  bool removeChild(const std::string& name) override;

  std::shared_ptr<DataFile> insertDataFile(const std::string& name,
                                           mode_t mode) override {
    auto child = getBackend()->createFile(mode);
    insertChild(name, child);
    return child;
  }

  std::shared_ptr<Directory> insertDirectory(const std::string& name,
                                             mode_t mode) override {
    auto child = getBackend()->createDirectory(mode);
    insertChild(name, child);
    return child;
  }

  std::shared_ptr<Symlink> insertSymlink(const std::string& name,
                                         const std::string& target) override {
    auto child = getBackend()->createSymlink(target);
    insertChild(name, child);
    return child;
  }

  bool insertMove(const std::string& name, std::shared_ptr<File> file) override;

  size_t getNumEntries() override { return entries.size(); }
  std::vector<Directory::Entry> getEntries() override;

public:
  MemoryDirectory(mode_t mode, backend_t backend) : Directory(mode, backend) {}
};

class MemorySymlink : public Symlink {
  std::string target;

  std::string getTarget() const override { return target; }

public:
  MemorySymlink(const std::string& target, backend_t backend)
    : Symlink(backend), target(target) {}
};

backend_t createMemoryFileBackend();

} // namespace wasmfs
