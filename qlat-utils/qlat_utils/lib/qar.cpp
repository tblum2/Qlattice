#include <qlat-utils/qar.h>

namespace qlat
{  //

const std::string qar_header = "#!/usr/bin/env qar-glimpse\n\n";

const std::string qar_idx_header = "#!/usr/bin/env qar-idx-glimpse\n\n";

static void add_qfile(const QFile& qfile);

static void remove_qfile(const QFile& qfile);

static void clean_up_qfile_map();

static bool operator==(const QarSegmentInfo& qsinfo1, const QarSegmentInfo& qsinfo2);

static bool operator!=(const QarSegmentInfo& qsinfo1, const QarSegmentInfo& qsinfo2);

static bool register_file(const QarFileVol& qar, const std::string& fn,
                          const QarSegmentInfo& qsinfo);

static QFile read_next(const QarFileVol& qar, std::string& fn);

static void read_through(const QarFileVol& qar);

static bool read_qar_segment_info(QarFileVolObj& qar, QarSegmentInfo& qsinfo);

static std::string read_fn(const QarFileVol& qar, const QarSegmentInfo& qsinfo);

static std::string read_info(const QarFileVol& qar, const QarSegmentInfo& qsinfo);

static QFile get_qfile_of_data(const QarFileVol& qar,
                               const QarSegmentInfo& qsinfo);

static bool verify_segment(const QarFileVol& qar, const std::string& fn);

static bool verify_index(const QarFileVol& qar);

static std::string qar_file_multi_vol_suffix(const Long i);

static void qar_check_if_create_new_vol(QarFile& qar, const Long data_size);

static std::string mk_key_from_qar_path(const std::string& path);

static std::string mk_new_qar_read_cache_key(const QarFile& qar,
                                             const std::string& key,
                                             const std::string& path);

static std::string mk_new_qar_read_cache_key(const std::string& path);

static std::string get_qar_read_cache_key(const std::string& path);

static void check_all_files_crc32_aux(
    std::vector<std::pair<std::string, crc32_t>>& acc, const std::string& path);

// ----------------------------------------------------

Long QFileBase::size()
{
  if (null()) {
    return -1;
  }
  const Long offset_start = tell();
  seek(0, SEEK_END);
  const Long offset_end = tell();
  seek(offset_start, SEEK_SET);
  qassert(offset_end >= 0);
  return offset_end;
}

Long QFileBase::remaining_size()
{
  if (null()) {
    return -1;
  }
  const Long offset_start = tell();
  seek(0, SEEK_END);
  const Long offset_end = tell();
  seek(offset_start, SEEK_SET);
  qassert(offset_end >= 0);
  const Long data_len = offset_end - offset_start;
  qassert(data_len >= 0);
  return data_len;
}

Long QFileBase::read_data(Vector<char> v)
{
  TIMER_FLOPS("QFileBase::read_data(v)");
  qassert(not null());
  qassert(mode() == QFileMode::Read);
  const Long total_bytes = read((void*)v.data(), sizeof(char), v.size());
  if (total_bytes > v.size()) {
    qerr(
        fname +
        ssprintf(": qread_data data_size=%ld total_bytes=%ld qfile.path()='%s'",
                 v.size(), total_bytes, path().c_str()));
  }
  timer.flops += total_bytes;
  return total_bytes;
}

std::string QFileBase::read_all()
{
  TIMER_FLOPS("QFileBase::read_all()");
  qassert(not null());
  qassert(mode() == QFileMode::Read);
  const Long total_bytes_expect = remaining_size();
  std::string ret;
  ret.resize(total_bytes_expect);
  const Long total_bytes = read_data(get_data_char(ret));
  if (total_bytes != total_bytes_expect) {
    qerr(fname + ssprintf(": read size=%ld expect size=%ld path()='%s'",
                          total_bytes, total_bytes_expect, path().c_str()));
  }
  timer.flops += total_bytes;
  return ret;
}

std::string QFileBase::cat()
{
  TIMER_FLOPS("QFileBase::cat()");
  seek(0, SEEK_END);
  const Long total_bytes_expect = tell();
  seek(0, SEEK_SET);
  std::string ret;
  ret.resize(total_bytes_expect);
  const Long total_bytes = read_data(get_data_char(ret));
  if (total_bytes != total_bytes_expect) {
    qerr(fname + ssprintf(": read size=%ld expect size=%ld path()='%s'",
                          total_bytes, total_bytes_expect, path().c_str()));
  }
  timer.flops += total_bytes;
  return ret;
}

std::string QFileBase::getline()
// read until '\n' or EOF
// include the final '\n'
{
  TIMER_FLOPS("QFileBase::getline()");
  qassert(not null());
  qassert(mode() == QFileMode::Read);
  const Long chunk_size = 128;
  std::string ret;
  std::string buf(chunk_size, ' ');
  const Long offset_initial = tell();
  while (true) {
    const Long len = read_data(get_data_char(buf));
    qassert(len <= chunk_size);
    for (int i = 0; i < len; ++i) {
      if (buf[i] == '\n') {
        ret += buf.substr(0, i + 1);
        seek(offset_initial + ret.size(), SEEK_SET);
        timer.flops += ret.size();
        return ret;
      }
    }
    if (len < chunk_size) {
      ret += buf.substr(0, len);
      timer.flops += ret.size();
      return ret;
    } else {
      qassert(len == chunk_size);
      ret += buf;
    }
  }
  return ret;
}

std::vector<std::string> QFileBase::getlines()
{
  TIMER_FLOPS("QFileBase::getlines()");
  qassert(not null());
  qassert(mode() == QFileMode::Read);
  std::vector<std::string> ret;
  while (not eof()) {
    ret.push_back(getline());
    timer.flops += ret.back().size();
  }
  return ret;
}

Long QFileBase::write_data(Vector<char> v)
{
  TIMER_FLOPS("QFileBase::write_data()");
  qassert(not null());
  qassert(mode() == QFileMode::Write or mode() == QFileMode::Append);
  const Long total_bytes = write((void*)v.data(), sizeof(char), v.size());
  if (total_bytes != v.size()) {
    qwarn(fname +
          ssprintf(": qwrite_data data_size=%ld total_bytes=%ld path()='%s'",
                   v.size(), total_bytes, path().c_str()));
  }
  timer.flops += total_bytes;
  return total_bytes;
}

Long QFileBase::write_data(const std::string& v)
{
  qassert(not null());
  return write_data(get_data_char(v));
}

Long QFileBase::write_data(const std::vector<std::string>& v)
{
  TIMER_FLOPS("qwrite_data(vec<str>)");
  qassert(not null());
  Long total_bytes = 0;
  for (Long i = 0; i < (Long)v.size(); ++i) {
    total_bytes += write_data(v[i]);
  }
  return total_bytes;
}

int QFileBase::append(const std::string& content)
{
  TIMER_VERBOSE_FLOPS("QFileBase::append(content)");
  const Long total_bytes = write_data(content);
  const Long total_bytes_expect = content.size();
  timer.flops += total_bytes;
  if (total_bytes != total_bytes_expect) {
    return 1;
  }
  return 0;
}

int QFileBase::append(const std::vector<std::string>& content)
{
  TIMER_VERBOSE_FLOPS("QFileBase::append(content)");
  qassert(not null());
  for (Long i = 0; i < (Long)content.size(); ++i) {
    const Long total_bytes = write_data(get_data_char(content[i]));
    const Long total_bytes_expect = content[i].size();
    timer.flops += total_bytes;
    if (total_bytes != total_bytes_expect) {
      return 1;
    }
  }
  return 0;
}

Long QFileBase::vprintf(const char* fmt, va_list args)
{
  TIMER("QFileBase::vprintf");
  const std::string str = vssprintf(fmt, args);
  return write_data(str);
}

Long QFileBase::printf(const char* fmt, ...)
{
  TIMER("QFileBase::printf");
  va_list args;
  va_start(args, fmt);
  return vprintf(fmt, args);
}

// ----------------------------------------------------

std::string show(const QFileMode mode)
{
  if (mode == QFileMode::Read) {
    return "r";
  } else if (mode == QFileMode::Write) {
    return "w";
  } else if (mode == QFileMode::Append) {
    return "a";
  } else {
    qassert(false);
    return "";
  }
}

QFileMode read_qfile_mode(const std::string& mode)
{
  if (mode == "r") {
    return QFileMode::Read;
  } else if (mode == "w") {
    return QFileMode::Write;
  } else if (mode == "a") {
    return QFileMode::Append;
  } else {
    qassert(false);
    return QFileMode::Read;
  }
}

Long write_from_qfile(QFileBase& qfile_out, QFileBase& qfile_in)
{
  TIMER_FLOPS("write_from_qfile(qfile_out,qfile_in)");
  const Long chunk_size = write_from_qfile_chunk_size();
  std::vector<char> buf(chunk_size);
  Long total_bytes = 0;
  while (not qfile_in.eof()) {
    const Long size = qfile_in.read_data(get_data(buf));
    qassert(size <= chunk_size);
    const Long size_out = qfile_out.write_data(get_data(get_data(buf), size));
    qassert(size_out == size);
    total_bytes += size;
  }
  timer.flops += total_bytes;
  return total_bytes;
}

// ----------------------------------------------------

QFileObjCFile::QFileObjCFile()
{
  fp = NULL;
  init();
}

QFileObjCFile::QFileObjCFile(const std::string& path_, const QFileMode mode_)
{
  fp = NULL;
  init(path_, mode_);
}

QFileObjCFile::~QFileObjCFile() { close(); }

void QFileObjCFile::init()
{
  close();
  path_v = "";
  mode_v = QFileMode::Read;
}

void QFileObjCFile::init(const std::string& path_, const QFileMode mode_)
{
  close();
  path_v = path_;
  mode_v = mode_;
  fp = fopen(path_v.c_str(), show(mode_v).c_str());
}

void QFileObjCFile::close()
{
  if (fp == NULL) {
    return;
  }
  TIMER("QFileObjCFile::close");
  int ret = std::fclose(fp);
  if (ret != 0) {
    qwarn(fname +
          ssprintf(": '%s' %s failed.", path().c_str(), show(mode()).c_str()));
  }
  fp = NULL;
}

QFileType QFileObjCFile::ftype() const { return QFileType::CFile; }

const std::string& QFileObjCFile::path() const { return path_v; }

QFileMode QFileObjCFile::mode() const { return mode_v; }

bool QFileObjCFile::null() const { return fp == NULL; }

bool QFileObjCFile::eof() const
{
  qassert(not null());
  return std::feof(fp);
}

Long QFileObjCFile::tell() const
{
  qassert(not null());
  return std::ftell(fp);
}

int QFileObjCFile::flush() const
{
  qassert(not null());
  return fflush(fp);
}

int QFileObjCFile::seek(const Long offset, const int whence)
{
  qassert(not null());
  return std::fseek(fp, offset, whence);
}

Long QFileObjCFile::read(void* ptr, const Long size, const Long nmemb)
{
  qassert(not null());
  return fread(ptr, size, nmemb, fp);
}

Long QFileObjCFile::write(const void* ptr, const Long size, const Long nmemb)
{
  qassert(not null());
  return fwrite(ptr, size, nmemb, fp);
}

// ----------------------------------------------------

QFileObj::QFileObj()
{
  number_of_child = 0;
  init();
}

QFileObj::QFileObj(const std::string& path_, const QFileMode mode_)
{
  number_of_child = 0;
  init(path_, mode_);
}

QFileObj::QFileObj(const std::shared_ptr<QFileObj>& qfile,
                   const Long q_offset_start, const Long q_offset_end)
{
  number_of_child = 0;
  init(qfile, q_offset_start, q_offset_end);
}

QFileObj::QFileObj(QFileObj&& qfile) noexcept
{
  number_of_child = 0;
  init();
  qswap(*this, qfile);
}

QFileObj::~QFileObj()
{
  close();
}

void QFileObj::init()
{
  close();
  pos = 0;
  is_eof = false;
  offset_start = 0;
  offset_end = -1;
}

void QFileObj::init(const std::string& path_, const QFileMode mode_)
{
  close();
  displayln_info(1, ssprintf("QFile: open '%s' with '%s'.", path_.c_str(),
                             show(mode_).c_str()));
  if (mode_ == QFileMode::Read and (not is_regular_file(path_))) {
    qwarn(ssprintf("QFile: open '%s' with '%s'. Not regular file.",
                   path_.c_str(), show(mode_).c_str()));
  }
  qassert(fp == nullptr);
  fp.reset(new QFileObjCFile(path_, mode_));
  if (fp->null()) {
    fp = nullptr;
    qwarn(ssprintf("QFile: open '%s' with '%s' failed.", path_.c_str(),
                   show(mode_).c_str()));
    pos = 0;
  } else {
    pos = fp->tell();
  }
  is_eof = false;
  offset_start = 0;
  offset_end = -1;
  if (not fp->null() and mode_ == QFileMode::Read) {
    offset_end = fp->size();
  }
}

void QFileObj::init(const std::shared_ptr<QFileObj>& qfile,
                    const Long q_offset_start, const Long q_offset_end)
// Become a child of qfile.
// NOTE: q_offset_start and q_offset_end are relative offset for qfile not the
// absolute offset for qfile.fp .
// q_offset_end == -1 means no additional limit
// NOTE: Initial position set to be 0. Does not perform fseek to appropriate
// position.
{
  close();
  if (qfile == nullptr) {
    return;
  }
  parent = qfile;
  parent->number_of_child += 1;
  qassert(fp == nullptr);
  fp = parent->fp;
  qassert(q_offset_start >= 0);
  is_eof = false;
  pos = 0;
  offset_start = parent->offset_start + q_offset_start;
  if (q_offset_end == -1) {
    offset_end = parent->offset_end;
  } else {
    qassert(q_offset_end >= q_offset_start);
    offset_end = parent->offset_start + q_offset_end;
    if (parent->offset_end != -1) {
      qassert(offset_end <= parent->offset_end);
    }
  }
  if (fp->mode() == QFileMode::Read and offset_end != -1) {
    const int code = fp->seek(offset_end, SEEK_SET);
    if (code != 0) {
      qwarn(ssprintf("QFile: '%s' with '%s' offset=%ld,%ld failed.",
                     fp->path().c_str(), show(fp->mode()).c_str(), offset_start,
                     offset_end));
      close();
    }
  }
}

void QFileObj::close()
{
  // to close the file, it cannot have any child
  qassert(number_of_child == 0);
  if (parent == nullptr) {
    if (fp != nullptr) {
      displayln_info(1, ssprintf("QFile: close '%s' with '%s'.", path().c_str(),
                                 show(mode()).c_str()));
      fp->close();
      fp.reset();
    }
  } else {
    fp.reset();
    parent->number_of_child -= 1;
    parent.reset();
  }
  qassert(fp == nullptr);
  qassert(parent == nullptr);
}

QFileType QFileObj::ftype() const
{
  qassert(not null());
  return fp->ftype();
}

const std::string& QFileObj::path() const
{
  qassert(not null());
  return fp->path();
}

QFileMode QFileObj::mode() const
{
  qassert(not null());
  return fp->mode();
}

bool QFileObj::null() const
{
  if (fp == nullptr) {
    return true;
  } else {
    qassert(not fp->null());
    return false;
  }
}

bool QFileObj::eof() const
{
  qassert(not null());
  return is_eof;
}

Long QFileObj::tell() const
{
  qassert(not null());
  return pos;
}

int QFileObj::flush() const
{
  qassert(not null());
  return fp->flush();
}

int QFileObj::seek(const Long q_offset, const int whence)
{
  qassert(not null());
  is_eof = false;
  int ret = 0;
  if (SEEK_SET == whence) {
    const Long offset = offset_start + q_offset;
    ret = fp->seek(offset, SEEK_SET);
  } else if (SEEK_CUR == whence) {
    ret = fp->seek(offset_start + pos + q_offset, SEEK_SET);
  } else if (SEEK_END == whence) {
    if (offset_end == -1) {
      ret = fp->seek(q_offset, SEEK_END);
    } else {
      const Long offset = offset_end + q_offset;
      ret = fp->seek(offset, SEEK_SET);
    }
  } else {
    qassert(false);
  }
  pos = fp->tell() - offset_start;
  qassert(pos >= 0);
  if (offset_end != -1) {
    qassert(offset_start + pos <= offset_end);
  }
  return ret;
}

Long QFileObj::read(void* ptr, const Long size, const Long nmemb)
// Only read portion of data if not enough content in qfile.
{
  qassert(not null());
  qassert(mode() == QFileMode::Read);
  if (0 == size or 0 == nmemb) {
    return 0;
  }
  qassert(size > 0);
  qassert(nmemb > 0);
  const int code = seek(pos, SEEK_SET);
  qassert(code == 0);
  Long actual_nmemb = 0;
  if (offset_end != -1) {
    const Long remaining_size = offset_end - offset_start - pos;
    qassert(remaining_size >= 0);
    const Long target_nmemb = std::min(nmemb, remaining_size / size);
    actual_nmemb = fp->read(ptr, size, target_nmemb);
    qassert(actual_nmemb == target_nmemb);
  } else {
    actual_nmemb = fp->read(ptr, size, nmemb);
  }
  pos += actual_nmemb * size;
  qassert(pos >= 0);
  if (actual_nmemb < nmemb) {
    is_eof = true;
  } else {
    qassert(actual_nmemb == nmemb);
    is_eof = false;
  }
  qassert(offset_start + pos == fp->tell());
  if (offset_end != -1) {
    qassert(offset_start + pos <= offset_end);
    if (is_eof) {
      qassert(offset_start + pos == offset_end);
    }
  } else {
    qassert(is_eof == fp->eof());
  }
  return actual_nmemb;
}

Long QFileObj::write(const void* ptr, const Long size, const Long nmemb)
// Crash if no enough space
{
  qassert(not null());
  qassert(mode() == QFileMode::Write or mode() == QFileMode::Append);
  if (0 == size or 0 == nmemb) {
    return 0;
  }
  qassert(size > 0);
  qassert(nmemb > 0);
  const int code = seek(pos, SEEK_SET);
  qassert(code == 0);
  if (offset_end != -1) {
    const Long remaining_size = offset_end - offset_start - pos;
    qassert(remaining_size >= size * nmemb);
  }
  const Long actual_nmemb = fp->write(ptr, size, nmemb);
  qassert(actual_nmemb == nmemb);
  pos += actual_nmemb * size;
  qassert(pos >= 0);
  qassert(offset_start + pos == fp->tell());
  if (offset_end != -1) {
    qassert(offset_start + pos <= offset_end);
  }
  return actual_nmemb;
}

// ----------------------------------------------------

QFile::QFile() { init(); }

QFile::QFile(const std::weak_ptr<QFileObj>& wp) { init(wp); }

QFile::QFile(const std::string& path, const QFileMode mode)
{
  init(path, mode);
}

QFile::QFile(const QFile& qfile, const Long q_offset_start,
             const Long q_offset_end)
{
  init(qfile, q_offset_start, q_offset_end);
}

void QFile::init() { p = nullptr; }

void QFile::init(const std::weak_ptr<QFileObj>& wp)
{
  TIMER("QFile::init(wp)");
  close();
  p = std::shared_ptr<QFileObj>(wp);
  if (p->null()) {
    close();
  }
}

void QFile::init(const std::string& path, const QFileMode mode)
{
  TIMER("QFile::init(path,mode)");
  close();
  p = std::shared_ptr<QFileObj>(new QFileObj());
  p->init(path, mode);
  if (p->null()) {
    close();
  } else {
    add_qfile(*this);
  }
}

void QFile::init(const QFile& qfile, const Long q_offset_start,
                 const Long q_offset_end)
{
  TIMER("QFile::init(qfile,offset_start,offset_end)");
  close();
  p = std::shared_ptr<QFileObj>(new QFileObj());
  p->init(qfile.p, q_offset_start, q_offset_end);
  if (p->null()) {
    close();
  } else {
    add_qfile(*this);
  }
}

void QFile::close()
{
  if (p != nullptr) {
    TIMER("QFile::close()");
    p->close();
    qassert(p->null());
    remove_qfile(*this);
    p = nullptr;
  }
  qassert(p == nullptr);
}

QFileType QFile::ftype() const
{
  qassert(not null());
  return p->ftype();
}

const std::string& QFile::path() const
{
  qassert(not null());
  return p->path();
}

QFileMode QFile::mode() const
{
  qassert(not null());
  return p->mode();
}

bool QFile::null() const
{
  if (p == nullptr) {
    return true;
  } else {
    qassert(not p->null());
    return false;
  }
}

bool QFile::eof() const
{
  qassert(not null());
  return p->eof();
}

Long QFile::tell() const
{
  qassert(not null());
  return p->tell();
}

int QFile::flush() const
{
  qassert(not null());
  return p->flush();
}

int QFile::seek(const Long offset, const int whence)
{
  qassert(not null());
  return p->seek(offset, whence);
}

Long QFile::read(void* ptr, const Long size, const Long nmemb)
{
  qassert(not null());
  return p->read(ptr, size, nmemb);
}

Long QFile::write(const void* ptr, const Long size, const Long nmemb)
{
  qassert(not null());
  return p->write(ptr, size, nmemb);
}

// ----------------------------------------------------

void add_qfile(const QFile& qfile)
{
  TIMER("add_qfile");
  QFileMap& qfile_map = get_all_qfile();
  const Long key = (Long)qfile.p.get();
  if (has(qfile_map, key)) {
    qwarn(fname + ssprintf(": repeatedly add qfile '%s' '%s' total qfiles %ld",
                           qfile.path().c_str(), show(qfile.mode()).c_str(),
                           (long)qfile_map.size()));
  }
  qfile_map[key] = qfile.p;
}

void remove_qfile(const QFile& qfile)
{
  TIMER("remove_qfile");
  if (qfile.p == nullptr) {
    return;
  }
  if (qfile.p->null()) {
    QFileMap& qfile_map = get_all_qfile();
    const Long key = (Long)qfile.p.get();
    qassert(has(qfile_map, key));
    qfile_map.erase(key);
  }
}

void clean_up_qfile_map()
{
  TIMER("clean_up_qfile_map");
  QFileMap& qfile_map = get_all_qfile();
  std::vector<Long> key_to_remove_vec;
  for (auto it = qfile_map.cbegin(); it != qfile_map.cend(); ++it) {
    if (it->second.expired()) {
      // qwarn(fname + ssprintf(": plan to remove expired entries %ld.",
      // (long)it->first));
      key_to_remove_vec.push_back(it->first);
    }
  }
  for (Long i = 0; i < (Long)key_to_remove_vec.size(); ++i) {
    const Long key = key_to_remove_vec[i];
    // qwarn(fname + ssprintf(": removing expired entries %ld.", (long)key));
    qfile_map.erase(key);
  }
  if (key_to_remove_vec.size() > 0) {
    qwarn(fname +
          ssprintf(": clean up %ld qfiles in qfile_map. %ld qfiles remain.",
                   (long)key_to_remove_vec.size(), (long)qfile_map.size()));
  }
}

std::vector<std::string> show_all_qfile()
{
  TIMER_VERBOSE("show_all_qfile");
  clean_up_qfile_map();
  std::vector<std::string> ret;
  const QFileMap& qfile_map = get_all_qfile();
  for (auto it = qfile_map.cbegin(); it != qfile_map.cend(); ++it) {
    QFile qfile = QFile(it->second);
    ret.push_back(show(qfile));
  }
  return ret;
}

// ----------------------------------------------------

std::string show(const QFileObj& qfile)
{
  const std::string has_parent = qfile.parent == nullptr ? "no" : "yes";
  return ssprintf("QFileObj(path='%s',mode='%s',parent=%s,number_of_child=%d)",
                  qfile.path().c_str(), show(qfile.mode()).c_str(),
                  has_parent.c_str(), qfile.number_of_child);
}

void qswap(QFileObj& qfile1, QFileObj& qfile2)
{
  // cannot swap if has child
  qassert(qfile1.number_of_child == 0);
  qassert(qfile2.number_of_child == 0);
  std::swap(qfile1.fp, qfile2.fp);
  std::swap(qfile1.parent, qfile2.parent);
  std::swap(qfile1.number_of_child, qfile2.number_of_child);
  std::swap(qfile1.pos, qfile2.pos);
  std::swap(qfile1.is_eof, qfile2.is_eof);
  std::swap(qfile1.offset_start, qfile2.offset_start);
  std::swap(qfile1.offset_end, qfile2.offset_end);
}

std::string show(const QFile& qfile) { return show(*qfile.p); }

void qswap(QFile& qfile1, QFile& qfile2)
// interface function
{
  std::swap(qfile1, qfile2);
}

QFile qfopen(const std::string& path, const QFileMode mode)
// interface function
// qfile.null() == true if qopen failed.
// Will open files in qar for read
// Will create directories needed for write / append
{
  TIMER("qfopen(path,mode)");
  if (mode == QFileMode::Read) {
    const std::string key = get_qar_read_cache_key(path);
    if (key == "") {
      return QFile();
    } else if (key == path) {
      return QFile(path, mode);
    } else {
      qassert(key == path.substr(0, key.size()));
      const std::string fn = path.substr(key.size());
      QarFile& qar = get_qar_read_cache()[key];
      QFile qfile = read(qar, fn);
      return qfile;
    }
  } else if (mode == QFileMode::Write or mode == QFileMode::Append) {
    const std::string path_dir = dirname(path);
    qmkdir_p(path_dir);
    return QFile(path, mode);
  } else {
    qassert(false);
  }
  return QFile();
}

QFile qfopen(const std::string& path, const std::string& mode)
{
  return qfopen(path, read_qfile_mode(mode));
}

void qfclose(QFile& qfile)
// interface function
{
  qfile.close();
}

bool qfeof(const QFile& qfile)
// interface function
{
  return qfile.eof();
}

Long qftell(const QFile& qfile)
// interface function
{
  return qfile.tell();
}

int qfflush(const QFile& qfile)
// interface function
{
  return qfile.flush();
}

int qfseek(QFile& qfile, const Long offset, const int whence)
{
  return qfile.seek(offset, whence);
}

int qfseek_set(QFile& qfile, const Long offset)
{
  return qfseek(qfile, offset, SEEK_SET);
}

int qfseek_end(QFile& qfile, const Long offset)
{
  return qfseek(qfile, offset, SEEK_END);
}

int qfseek_cur(QFile& qfile, const Long offset)
{
  return qfseek(qfile, offset, SEEK_CUR);
}

Long qfread(void* ptr, const Long size, const Long nmemb, QFile& qfile)
// interface function
// Only read portion of data if not enough content in qfile.
{
  return qfile.read(ptr, size, nmemb);
}

Long qfwrite(const void* ptr, const Long size, const Long nmemb, QFile& qfile)
// interface function
// Crash if no enough space
{
  return qfile.write(ptr, size, nmemb);
}

Long qvfprintf(QFile& qfile, const char* fmt, va_list args)
{
  return qfile.vprintf(fmt, args);
}

Long qfprintf(QFile& qfile, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  return qfile.vprintf(fmt, args);
}

std::string qgetline(QFile& qfile)
// interface function
// read an entire line including the final '\n' char.
{
  return qfile.getline();
}

std::vector<std::string> qgetlines(QFile& qfile)
// interface function
{
  return qfile.getlines();
}

Long qfile_size(QFile& qfile)
// interface function
// return the total size of qfile.
// qfile should have definite size.
// does not affect qfile position.
// return -1 if qFile is not opened.
{
  return qfile.size();
}

Long qfile_remaining_size(QFile& qfile)
// interface function
// return the remaining size of qfile (start from the current position).
// qfile should have definite size.
// does not affect qfile position.
// return -1 if qfile is not opened.
{
  return qfile.remaining_size();
}

Long qwrite_data(const Vector<char>& v, QFile& qfile)
// interface function
{
  return qfile.write_data(v);
}

Long qwrite_data(const std::string& v, QFile& qfile)
// interface function
{
  return qfile.write_data(v);
}

Long qwrite_data(const std::vector<std::string>& v, QFile& qfile)
// interface function
{
  return qfile.write_data(v);
}

Long qread_data(const Vector<char>& v, QFile& qfile)
// interface function
{
  return qfile.read_data(v);
}

std::string qcat(QFile& qfile)
{
  return qfile.cat();
}

int qappend(QFile& qfile, const std::string& content)
{
  return qfile.append(content);
}

int qappend(QFile& qfile, const std::vector<std::string>& content)
{
  return qfile.append(content);
}

// ----------------------------------------------------

void QarSegmentInfo::update_offset()
{
  offset_info = offset_fn + fn_len + 1;
  offset_data = offset_info + info_len + 1;
  offset_end = offset_data + data_len + 2;
}

bool QarSegmentInfo::check_offset()
{
  const int header_min_len = 14;
  if (offset_fn < offset + header_min_len + 1) {
    return false;
  }
  if (offset_info != offset_fn + fn_len + 1) {
    return false;
  }
  if (offset_data != offset_info + info_len + 1) {
    return false;
  }
  if (offset_end != offset_data + data_len + 2) {
    return false;
  }
  return true;
}

// ----------------------------------------------------

void QarFileVolObj::init()
{
  qfile.init();
  is_read_through = false;
  fn_list.clear();
  qsinfo_map.clear();
  directories.clear();
  max_offset = 0;
  current_write_segment_fn = "";
  current_write_segment_info.init();
}

void QarFileVolObj::init(const std::string& path, const QFileMode mode)
{
  TIMER("QarFileVolObj::init(path,mode)");
  init();
  if (mode == QFileMode::Append) {
    properly_truncate_qar_vol_file(fn_list, qsinfo_map, directories, max_offset,
                                   path, false);
    qfile = QFile(path, mode);
  } else {
    init(QFile(path, mode));
  }
}

void QarFileVolObj::init(const QFile& qfile_)
{
  TIMER("QarFileVolObj::init(qfile)");
  init();
  qfile = qfile_;
  if (qfile.null()) {
    return;
  }
  qassert(qftell(qfile) == 0);
  if (mode() == QFileMode::Write or mode() == QFileMode::Append) {
    // write from scratch even if mode is Append (perhaps inherited from parent
    // qfile)
    // will append if use QarFileVolObj::init(path, QFileMode::Append)
    qfwrite(qar_header.data(), qar_header.size(), 1, qfile);
  } else if (mode() == QFileMode::Read) {
    std::vector<char> check_line(qar_header.size(), 0);
    const Long qfread_check_len =
        qfread(check_line.data(), qar_header.size(), 1, qfile);
    if (not(qfread_check_len == 1 and
            std::string(check_line.data(), check_line.size()) == qar_header)) {
      qfclose(qfile);
      qwarn(fname +
            ssprintf(": '%s' format does not match.", qfile.path().c_str()));
      return;
    };
    max_offset = qftell(qfile);
    directories.insert("");
  } else {
    qassert(false);
  }
}

void QarFileVolObj::close()
{
  qfclose(qfile);
  init();
}

// ----------------------------------------------------

void QarFileVol::init(const std::string& path, const QFileMode mode)
{
  TIMER("QarFileVol::init(path,mode)");
  if (p == nullptr) {
    p = std::shared_ptr<QarFileVolObj>(new QarFileVolObj());
  }
  p->init(path, mode);
}

void QarFileVol::init(const QFile& qfile)
{
  TIMER("QarFileVol::init(qfile)");
  if (p == nullptr) {
    p = std::shared_ptr<QarFileVolObj>(new QarFileVolObj());
  }
  p->init(qfile);
}

void QarFileVol::close()
{
  if (p != nullptr) {
    p->close();
    p = nullptr;
  }
}

const std::string& QarFileVol::path() const { return p->path(); }

QFileMode QarFileVol::mode() const { return p->mode(); }

QFile& QarFileVol::qfile() const { return p->qfile; }

// ----------------------------------------------------

bool operator==(const QarSegmentInfo& qsinfo1, const QarSegmentInfo& qsinfo2)
{
  if (qsinfo1.offset != qsinfo2.offset) {
    return false;
  } else if (qsinfo1.offset_fn != qsinfo2.offset_fn) {
    return false;
  } else if (qsinfo1.offset_data != qsinfo2.offset_data) {
    return false;
  } else if (qsinfo1.offset_end != qsinfo2.offset_end) {
    return false;
  } else if (qsinfo1.fn_len != qsinfo2.fn_len) {
    return false;
  } else if (qsinfo1.info_len != qsinfo2.info_len) {
    return false;
  } else if (qsinfo1.data_len != qsinfo2.data_len) {
    return false;
  } else {
    return true;
  }
}

bool operator!=(const QarSegmentInfo& qsinfo1, const QarSegmentInfo& qsinfo2)
{
  return not(qsinfo1 == qsinfo2);
}

std::string qar_file_multi_vol_suffix(const Long i)
{
  if (i == 0) {
    return "";
  } else {
    return ssprintf(".v%ld", i);
  }
  qassert(false);
  return "";
}

bool register_file(const QarFileVol& qar, const std::string& fn,
                   const QarSegmentInfo& qsinfo)
{
  TIMER("register_file");
  if (not has(qar.p->qsinfo_map, fn)) {
    qar.p->fn_list.push_back(fn);
    qar.p->qsinfo_map[fn] = qsinfo;
    std::string dir = dirname(fn);
    while (dir != ".") {
      if (has(qar.p->directories, dir)) {
        break;
      } else {
        qar.p->directories.insert(dir);
        dir = dirname(dir);
      }
    }
    if (qar.p->max_offset < qsinfo.offset_end) {
      qar.p->max_offset = qsinfo.offset_end;
    }
  } else {
    if (qar.p->qsinfo_map[fn] != qsinfo) {
      qwarn(fname + ssprintf(": qar at '%s' wrong info for '%s'",
                             qar.path().c_str(), fn.c_str()));
      qar.p->qsinfo_map[fn] = qsinfo;
      return false;
    }
  }
  return true;
}

bool read_qar_segment_info(QarFileVolObj& qar, QarSegmentInfo& qsinfo)
// Initial pos: beginning of the segment, just before FILE-HEADER.
// Final pos: at the end of the segment, at the beginning of the next segment.
// Return true if read successfully (also qfseek to the beginning of the next
// segment).
{
  TIMER("read_qar_segment_info(qar_v,qsinfo)");
  qassert(not qar.null());
  qassert(qar.qfile.mode() == QFileMode::Read);
  set_zero(get_data_one_elem(qsinfo));
  if (qar.qfile.null()) {
    qwarn(ssprintf("read_tag: fn='%s' pos=%ld.", qar.qfile.path().c_str(),
                   qftell(qar.qfile)));
    return false;
  }
  qsinfo.offset = qftell(qar.qfile);
  const std::string header_prefix = "QAR-FILE ";
  const std::string header = qgetline(qar.qfile);
  if (header.size() == 0) {
    qar.is_read_through = true;
    return false;
  }
  if (header.size() <= header_prefix.size()) {
    qwarn(ssprintf("read_tag: fn='%s' pos=%ld.", qar.qfile.path().c_str(),
                   qftell(qar.qfile)));
    qar.is_read_through = true;
    return false;
  }
  if (header.substr(0, header_prefix.size()) != header_prefix) {
    qwarn(ssprintf("read_tag: fn='%s' pos=%ld header='%s'.",
                   qar.qfile.path().c_str(), qftell(qar.qfile),
                   header.c_str()));
    qar.is_read_through = true;
    return false;
  }
  const std::vector<Long> len_vec =
      read_longs(header.substr(header_prefix.size()));
  if (len_vec.size() != 3) {
    qwarn(ssprintf("read_tag: fn='%s' pos=%ld.", qar.qfile.path().c_str(),
                   qftell(qar.qfile)));
    qar.is_read_through = true;
    return false;
  }
  qsinfo.offset_fn = qftell(qar.qfile);
  qassert(
      qsinfo.offset_fn ==
      qsinfo.offset +
          (Long)header.size());  // this header str include the final '\n' char.
  qsinfo.fn_len = len_vec[0];
  qsinfo.info_len = len_vec[1];
  qsinfo.data_len = len_vec[2];
  qsinfo.update_offset();
  const int code = qfseek(qar.qfile, qsinfo.offset_end, SEEK_SET);
  if (code != 0) {
    qwarn(ssprintf("read_tag: fn='%s' pos=%ld offset_end=%ld.",
                   qar.qfile.path().c_str(), qftell(qar.qfile),
                   qsinfo.offset_end));
    qar.is_read_through = true;
    return false;
  }
  return true;
}

std::string read_fn(const QarFileVol& qar, const QarSegmentInfo& qsinfo)
{
  TIMER("read_fn(qar_v,qsinfo)");
  qassert(not qar.null());
  qassert(qar.mode() == QFileMode::Read);
  std::vector<char> data(qsinfo.fn_len);
  const int code = qfseek(qar.qfile(), qsinfo.offset_fn, SEEK_SET);
  qassert(code == 0);
  if (1 != qfread(data.data(), qsinfo.fn_len, 1, qar.qfile())) {
    qassert(false);
  }
  std::string fn;
  fn = std::string(data.data(), qsinfo.fn_len);
  return fn;
}

std::string read_info(const QarFileVol& qar, const QarSegmentInfo& qsinfo)
{
  TIMER("read_info(qar_v,qsinfo)");
  qassert(not qar.null());
  qassert(qar.mode() == QFileMode::Read);
  std::vector<char> data(qsinfo.info_len);
  const int code = qfseek(qar.qfile(), qsinfo.offset_info, SEEK_SET);
  qassert(code == 0);
  if (1 != qfread(data.data(), qsinfo.info_len, 1, qar.qfile())) {
    qassert(false);
  }
  std::string info;
  info = std::string(data.data(), qsinfo.info_len);
  return info;
}

QFile get_qfile_of_data(const QarFileVol& qar, const QarSegmentInfo& qsinfo)
// interface function
// set qfile to be a qfile containing the data specified by qsinfo.
// qfile initial pos is zero
{
  TIMER("get_qfile_of_data(qar_v,qsinfo)");
  qassert(not qar.null());
  qassert(qar.mode() == QFileMode::Read);
  QFile qfile(qar.qfile(), qsinfo.offset_data,
              qsinfo.offset_data + qsinfo.data_len);
  qassert(not qfile.null());
  return qfile;
}

bool verify_segment(const QarFileVol& qar, const std::string& fn)
{
  TIMER("verify_segment(qar_v,fn)");
  qassert(has(qar.p->qsinfo_map, fn));
  const QarSegmentInfo& qsinfo = qar.p->qsinfo_map[fn];
  qfseek(qar.p->qfile, qsinfo.offset, SEEK_SET);
  QarSegmentInfo qsinfo_read;
  if (not read_qar_segment_info(*qar.p, qsinfo_read)) {
    return false;
  }
  if (qsinfo != qsinfo_read) {
    return false;
  }
  const std::string fn_read = read_fn(qar, qsinfo);
  if (fn != fn_read) {
    return false;
  }
  return true;
}

bool verify_index(const QarFileVol& qar)
{
  TIMER("verify_index(qar_v)");
  const std::vector<std::string>& fn_list = qar.p->fn_list;
  Long offset_end = qar_header.size();
  for (Long k = 0; k < (Long)fn_list.size(); ++k) {
    const std::string& fn = fn_list[k];
    if (not verify_segment(qar, fn)) {
      return false;
    }
    const QarSegmentInfo& qsinfo = qar.p->qsinfo_map[fn];
    if (qsinfo.offset != offset_end) {
      return false;
    }
    offset_end = qsinfo.offset_end;
  }
  if (offset_end != qar.p->max_offset) {
    return false;
  }
  const Long file_size = qfile_size(qar.p->qfile);
  if (file_size != qar.p->max_offset) {
    return false;
  }
  return true;
}

QFile read_next(const QarFileVol& qar, std::string& fn)
// interface function
// Initial pos of qar should be at the beginning of a segment.
// register_file only if qfseek to the end of the file is successful.
{
  TIMER("read_next(qar_v,fn)");
  qassert(not qar.null());
  qassert(qar.mode() == QFileMode::Read);
  QarSegmentInfo qsinfo;
  if (not read_qar_segment_info(*qar.p, qsinfo)) {
    fn = std::string();
    return QFile();
  }
  fn = read_fn(qar, qsinfo);
  QFile qfile = get_qfile_of_data(qar, qsinfo);
  const int code = qfseek(qar.qfile(), qsinfo.offset_end, SEEK_SET);
  if (code != 0) {
    qfile.init();
  }
  register_file(qar, fn, qsinfo);
  return qfile;
}

void read_through(const QarFileVol& qar)
{
  TIMER("read_through(qar_v)");
  qassert(not qar.null());
  qassert(qar.mode() == QFileMode::Read);
  if (qar.p->is_read_through) {
    return;
  }
  std::string fn;
  const int code = qfseek(qar.qfile(), qar.p->max_offset, SEEK_SET);
  qassert(code == 0);
  while (true) {
    const QFile qfile = read_next(qar, fn);
    if (qfile.null()) {
      break;
    }
  }
}

void write_start(const QarFileVol& qar, const std::string& fn,
                 const std::string& info, QFile& qfile_out, const Long data_len,
                 const Long header_len)
// interface function
// Initial pos should be the end of the qar
// Set the qfile_out to be a writable QFile to qar.
// When the final size of qfile_out is unknown (data_len == -1), header_len is
// reserved for header.
// Should call write_end(qar) after writing to qfile_out is finished.
{
  TIMER("write_start(qar_v)");
  qassert(not qar.null());
  qar.p->current_write_segment_fn = fn;
  QarSegmentInfo& qsinfo = qar.p->current_write_segment_info;
  qassert(qsinfo.offset == 0);
  qsinfo.offset = qftell(qar.qfile());
  qassert(qsinfo.offset > 0);
  qfseek(qar.qfile(), 0, SEEK_END);
  qassert(qftell(qar.qfile()) == qsinfo.offset);
  const std::string header_prefix = "QAR-FILE ";
  std::string header;
  header = ssprintf("%ld %ld %ld", fn.size(), info.size(), data_len);
  if (data_len < 0) {
    qassert(data_len == -1);
    qassert(header_len - (Long)header_prefix.size() >= (Long)header.size());
    const std::string header_pad(
        header_len - header_prefix.size() - header.size(), ' ');
    header = header_pad + header;
  }
  header = header_prefix + header;
  std::string meta;
  meta += header;
  meta += "\n";
  meta += fn;
  meta += "\n";
  meta += info;
  meta += "\n";
  qwrite_data(meta, qar.qfile());
  qsinfo.offset_fn = qsinfo.offset + header.size() + 1;
  qsinfo.fn_len = fn.size();
  qsinfo.info_len = info.size();
  qsinfo.data_len = data_len;
  qsinfo.update_offset();
  qassert(qsinfo.offset_data == qftell(qar.qfile()));
  const Long offset_start = qsinfo.offset_data;
  const Long offset_end = data_len == -1 ? -1 : offset_start + data_len;
  qfile_out.init(qar.qfile(), offset_start, offset_end);
}

void write_end(const QarFileVol& qar)
// interface function
// Call after finish writing to a qfile set by write_start (qfile should be
// closed already).
// Use the end of file as the end of the data.
// Will check / add data_len information in header.
// Finally, will write "\n\n" after the end of file.
{
  TIMER("write_end(qar_v)");
  qassert(not qar.null());
  QarSegmentInfo& qsinfo = qar.p->current_write_segment_info;
  qfseek(qar.qfile(), 0, SEEK_END);
  const Long offset_end = qftell(qar.qfile());
  qassert(qsinfo.offset > 0);
  qassert(qsinfo.offset_data > qsinfo.offset);
  if (qsinfo.data_len >= 0) {
    qassert(qsinfo.offset_data + qsinfo.data_len == offset_end);
  } else {
    qassert(qsinfo.data_len == -1);
    const Long header_len = qsinfo.offset_fn - qsinfo.offset - 1;
    const Long fn_len = qsinfo.fn_len;
    const Long info_len = qsinfo.info_len;
    qsinfo.data_len = offset_end - qsinfo.offset_data;
    qassert(qsinfo.data_len >= 0);
    const std::string header_prefix = "QAR-FILE ";
    std::string header =
        ssprintf("%ld %ld %ld", fn_len, info_len, qsinfo.data_len);
    qassert(header_len >= (Long)header_prefix.size() + (Long)header.size());
    const std::string header_pad(
        header_len - header_prefix.size() - header.size(), ' ');
    header = header_prefix + header_pad + header;
    qassert((Long)header.size() == header_len);
    qfseek(qar.qfile(), qsinfo.offset, SEEK_SET);
    qwrite_data(header, qar.qfile());
    qfseek(qar.qfile(), 0, SEEK_END);
  }
  qwrite_data("\n\n", qar.qfile());
  qsinfo.update_offset();
  qassert(qftell(qar.qfile()) == qsinfo.offset_end);
  register_file(qar, qar.p->current_write_segment_fn, qsinfo);
  qar.p->current_write_segment_fn = "";
  qsinfo.init();
}

// ----------------------------------------------------

std::vector<std::string> list(const QarFileVol& qar)
// interface function
{
  TIMER("list(qar)");
  if (qar.null()) {
    return std::vector<std::string>();
  }
  if (qar.mode() == QFileMode::Read) {
    read_through(qar);
  }
  return qar.p->fn_list;
}

bool has_regular_file(const QarFileVol& qar, const std::string& fn)
// interface function
{
  TIMER("has_regular_file(qar_v,fn)");
  qassert(not qar.null());
  if (qar.p->is_read_through or qar.mode() == QFileMode::Write or
      qar.mode() == QFileMode::Append) {
    return has(qar.p->qsinfo_map, fn);
  }
  QFile qfile = read(qar, fn);
  return not qfile.null();
}

bool has(const QarFileVol& qar, const std::string& fn)
// interface function
{
  TIMER("has(qar_v,fn)");
  qassert(not qar.null());
  if (has_regular_file(qar, fn)) {
    return true;
  } else {
    if (qar.mode() == QFileMode::Read) {
      qassert(qar.p->is_read_through);
    }
    return has(qar.p->directories, fn);
  }
}

QFile read(const QarFileVol& qar, const std::string& fn)
// interface function
{
  TIMER("read(qar_v,fn)");
  qassert(not qar.null());
  qassert(qar.mode() == QFileMode::Read);
  qassert(fn != "");
  QFile qfile_in;
  if (has(qar.p->qsinfo_map, fn)) {
    const QarSegmentInfo& qsinfo = qar.p->qsinfo_map[fn];
    qfile_in = get_qfile_of_data(qar, qsinfo);
    return qfile_in;
  }
  if (qar.p->is_read_through) {
    return qfile_in;
  }
  const int code = qfseek(qar.qfile(), qar.p->max_offset, SEEK_SET);
  qassert(code == 0);
  std::string fn_read;
  while (true) {
    qfile_in = read_next(qar, fn_read);
    if (qfile_in.null()) {
      return qfile_in;
    }
    if (fn == fn_read) {
      return qfile_in;
    }
  }
  return qfile_in;
}

std::string read_data(const QarFileVol& qar, const std::string& fn)
{
  TIMER_VERBOSE("read_data(qar_v,fn)");
  QFile qfile = read(qar, fn);
  return qcat(qfile);
}

std::string read_info(const QarFileVol& qar, const std::string& fn)
{
  TIMER("read_info(qar_v,fn)");
  qassert(not qar.null());
  qassert(qar.mode() == QFileMode::Read);
  if (not has(qar, fn)) {
    return "";
  }
  const QarSegmentInfo& qsinfo = qar.p->qsinfo_map[fn];
  return read_info(qar, qsinfo);
}

Long write_from_qfile(const QarFileVol& qar, const std::string& fn,
                      const std::string& info, QFile& qfile_in)
// interface function
// Write content (start from the current position) of qfile_in to qar.
// qfile_in should have definite size.
// NOTE: write_start and write_end can be used for more general usage
{
  TIMER_FLOPS("write_from_qfile(QarFileVol)");
  qassert(not qar.null());
  const Long data_len = qfile_remaining_size(qfile_in);
  qassert(data_len >= 0);
  QFile qfile_out;
  write_start(qar, fn, info, qfile_out, data_len);
  const Long total_bytes = write_from_qfile(qfile_out, qfile_in);
  write_end(qar);
  qassert(data_len == total_bytes);
  timer.flops += total_bytes;
  return total_bytes;
}

Long write_from_data(const QarFileVol& qar, const std::string& fn,
                     const std::string& info, const Vector<char> data)
// interface function
// Write content data to qar.
// NOTE: write_start and write_end can be used for more general usage
{
  TIMER_FLOPS("write_from_data(QarFileVol)");
  qassert(not qar.null());
  const Long data_len = data.size();
  qassert(data_len >= 0);
  QFile qfile_out;
  write_start(qar, fn, info, qfile_out, data_len);
  const Long total_bytes = qwrite_data(data, qfile_out);
  write_end(qar);
  qassert(data_len == total_bytes);
  timer.flops += total_bytes;
  return total_bytes;
}

Long write_from_data(QarFileVol& qar, const std::string& fn,
                     const std::string& info, const std::string& data)
{
  return write_from_data(qar, fn, info, get_data_char(data));
}

Long write_from_data(QarFileVol& qar, const std::string& fn,
                     const std::string& info,
                     const std::vector<std::string>& data)
{
  TIMER_FLOPS("write_from_data(QarFileVol)");
  qassert(not qar.null());
  Long data_len = 0;
  for (Long i = 0; i < (Long)data.size(); ++i) {
    data_len += data[i].size();
  }
  qassert(data_len >= 0);
  QFile qfile_out;
  write_start(qar, fn, info, qfile_out, data_len);
  const Long total_bytes = qwrite_data(data, qfile_out);
  write_end(qar);
  qassert(data_len == total_bytes);
  timer.flops += total_bytes;
  return total_bytes;
}

// ----------------------------------------------------

int truncate_qar_vol_file(const std::string& path,
                          const std::vector<std::string>& fns_keep)
// interface function
// return nonzero if failed.
// return 0 if truncated successfully.
// if fns_keep is empty, the resulting qar file should have and only have
// qar_header.
// qar_file ready to be appended after this call.
{
  TIMER_VERBOSE("truncate_qar_vol_file");
  QarFileVol qar(path, QFileMode::Read);
  if (qar.null()) {
    if (fns_keep.size() == 0) {
      qar.init(path, QFileMode::Write);
      return 0;
    } else {
      qwarn(fname + ssprintf(": fns_keep.size()=%ld", fns_keep.size()));
      return 1;
    }
  }
  const std::vector<std::string> fns = list(qar);
  if (fns.size() < fns_keep.size()) {
    qwarn(fname + ssprintf(": fns.size()=%ld fns_keep.size()=%ld", fns.size(),
                           fns_keep.size()));
    return 2;
  }
  for (Long i = 0; i < (Long)fns_keep.size(); ++i) {
    if (fns[i] != fns_keep[i]) {
      qwarn(fname + ssprintf(": fns[i]='%s' fns_keep[i]='%s'", fns[i].c_str(),
                             fns_keep[i].c_str()));
      return 3;
    }
  }
  Long offset_final = qar_header.size();
  if (fns_keep.size() > 0) {
    std::string fn_last = fns_keep.back();
    offset_final = qar.p->qsinfo_map[fn_last].offset_end;
  }
  qar.close();
  const int b = qtruncate(path, offset_final);
  if (b != 0) {
    qwarn(fname +
          ssprintf(": fns.size()=%ld fns_keep.size()=%ld offset_final=%ld",
                   fns.size(), fns_keep.size(), offset_final));
    return 4;
  }
  return 0;
}

void properly_truncate_qar_vol_file(
    std::vector<std::string>& fn_list,
    std::map<std::string, QarSegmentInfo>& qsinfo_map,
    std::set<std::string>& directories, Long& max_offset,
    const std::string& path, const bool is_only_check)
{
  TIMER_VERBOSE("properly_truncate_qar_vol_file");
  if (not does_file_exist_cache(path)) {
    fn_list.clear();
    qsinfo_map.clear();
    directories.clear();
    QarFileVol qar(path, QFileMode::Write);
    qar.close();
    return;
  }
  QarFileVol qar(path, QFileMode::Read);
  qassert(not qar.null());
  read_through(qar);
  fn_list = qar.p->fn_list;
  qsinfo_map = qar.p->qsinfo_map;
  directories = qar.p->directories;
  max_offset = qar.p->max_offset;
  const Long file_size = qfile_size(qar.p->qfile);
  qassert(file_size >= max_offset);
  qar.close();
  if (not is_only_check) {
    if (file_size > max_offset) {
      const int b = qtruncate(path, max_offset);
      qassert(b == 0);
    }
  }
}

std::vector<std::string> properly_truncate_qar_vol_file(
    const std::string& path, const bool is_only_check)
// interface function
// The resulting qar file should at least have qar_header.
// Should call this function before append.
// qar_file ready to be appended after this call.
{
  std::vector<std::string> fn_list;
  std::map<std::string, QarSegmentInfo> qsinfo_map;
  std::set<std::string> directories;
  Long max_offset;
  properly_truncate_qar_vol_file(fn_list, qsinfo_map, directories, max_offset,
                                 path, is_only_check);
  return fn_list;
}

// ----------------------------------------------------

void QarFile::init()
{
  path = "";
  mode = QFileMode::Read;
  std::vector<QarFileVol>& v = *this;
  qlat::clear(v);
}

void QarFile::init(const std::string& path_, const QFileMode mode_)
{
  init();
  path = path_;
  mode = mode_;
  if (mode == QFileMode::Read) {
    // maximally 1024 * 1024 * 1024 volumes
    for (Long iv = 0; iv < 1024 * 1024 * 1024; ++iv) {
      const std::string path_qar_v = path + qar_file_multi_vol_suffix(iv);
      if (not does_regular_file_exist_qar(path_qar_v)) {
        break;
      }
      push_back(qfopen(path_qar_v, mode));
      if (back().null()) {
        pop_back();
        break;
      }
    }
    if (does_regular_file_exist_qar(path + ".idx")) {
      load_qar_index(*this, path + ".idx");
    }
  } else if (mode == QFileMode::Append) {
    for (Long iv = 0; iv < 1024 * 1024 * 1024; ++iv) {
      const std::string path_qar_v = path + qar_file_multi_vol_suffix(iv);
      // try to open the first file regardless its existence
      if (iv != 0) {
        if (not does_file_exist_cache(path_qar_v)) {
          break;
        }
      }
      push_back(qfopen(path_qar_v, mode));
      if (back().null()) {
        pop_back();
        break;
      }
    }
  } else {
    qassert(false);
  }
}

void QarFile::close()
{
  QarFile& qar = *this;
  for (int i = 0; i < (int)qar.size(); ++i) {
    qar[i].close();
  }
  init();
}

// ----------------------------------------------------

std::vector<std::string> list(const QarFile& qar)
// interface function
{
  TIMER("list(qar)");
  if (qar.null()) {
    return std::vector<std::string>();
  }
  std::vector<std::string> fn_list;
  for (Long i = 0; i < (Long)qar.size(); ++i) {
    const QarFileVol& qar_v = qar[i];
    qassert(not qar_v.null());
    vector_append(fn_list, list(qar_v));
  }
  return fn_list;
}

bool has_regular_file(const QarFile& qar, const std::string& fn)
// interface function
{
  TIMER("has_regular_file(qar,fn)");
  for (Long i = 0; i < (Long)qar.size(); ++i) {
    const QarFileVol& qar_v = qar[i];
    qassert(not qar_v.null());
    if (has_regular_file(qar_v, fn)) {
      return true;
    }
  }
  return false;
}

bool has(const QarFile& qar, const std::string& fn)
// interface function
{
  TIMER("has(qar,fn)");
  for (Long i = 0; i < (Long)qar.size(); ++i) {
    const QarFileVol& qar_v = qar[i];
    qassert(not qar_v.null());
    if (has(qar_v, fn)) {
      return true;
    }
  }
  return false;
}

QFile read(const QarFile& qar, const std::string& fn)
// interface function
{
  TIMER("read(qar,fn)");
  qassert(qar.mode == QFileMode::Read);
  QFile qfile_in;
  for (Long i = 0; i < (Long)qar.size(); ++i) {
    const QarFileVol& qar_v = qar[i];
    qassert(not qar_v.null());
    qassert(qar_v.mode() == QFileMode::Read);
    qfile_in = read(qar_v, fn);
    if (not qfile_in.null()) {
      return qfile_in;
    }
  }
  return qfile_in;
}

std::string read_data(const QarFile& qar, const std::string& fn)
{
  TIMER_VERBOSE("read_data(qar,fn)");
  QFile qfile = read(qar, fn);
  return qcat(qfile);
}

std::string read_info(const QarFile& qar, const std::string& fn)
{
  TIMER("read_info(qar,fn)");
  qassert(qar.mode == QFileMode::Read);
  for (Long i = 0; i < (Long)qar.size(); ++i) {
    const QarFileVol& qar_v = qar[i];
    qassert(not qar_v.null());
    qassert(qar_v.mode() == QFileMode::Read);
    if (has_regular_file(qar_v, fn)) {
      return read_info(qar_v, fn);
    }
  }
  return "";
}

bool verify_index(const QarFile& qar)
{
  TIMER("verify_index(qar)");
  qassert(qar.mode == QFileMode::Read);
  for (int i = 0; i < (int)qar.size(); ++i) {
    if (not verify_index(qar[i])) {
      return false;
    }
  }
  return true;
}

void qar_check_if_create_new_vol(QarFile& qar, const Long data_size)
// make sure qar.back() is appendable after this call.
{
  TIMER("qar_check_if_create_new_vol");
  qassert(qar.mode == QFileMode::Append);
  qassert(not qar.null());
  const QarFileVol& qar_v = qar.back();
  qassert(not qar_v.null());
  qassert(qar_v.mode() == QFileMode::Append);
  const Long max_size = get_qar_multi_vol_max_size();
  if (max_size > 0 and qar_v.p->max_offset + data_size > max_size) {
    const Long iv = qar.size();
    const std::string path_qar_v1 = qar.path + qar_file_multi_vol_suffix(iv);
    QarFileVol qar_v1(path_qar_v1, QFileMode::Append);
    qassert(not qar_v1.null());
    qar.push_back(qar_v1);
  }
  qassert(not qar.back().null());
  qassert(qar.back().mode() == QFileMode::Append);
}

Long write_from_qfile(QarFile& qar, const std::string& fn,
                      const std::string& info, QFile& qfile_in)
// interface function
// Write content (start from the current position) of qfile_in to qar.
// qfile_in should have definite size.
{
  TIMER_VERBOSE_FLOPS("write_from_qfile(QarFile)");
  if (has_regular_file(qar, fn)) {
    qwarn(fname + ssprintf(": qar at '%s' already has fn='%s'. Append anyway.",
                           qar.path.c_str(), fn.c_str()));
  }
  const Long data_len = qfile_remaining_size(qfile_in);
  qassert(data_len >= 0);
  qar_check_if_create_new_vol(qar, data_len);
  const Long total_bytes = write_from_qfile(qar.back(), fn, info, qfile_in);
  timer.flops += total_bytes;
  return total_bytes;
}

Long write_from_data(QarFile& qar, const std::string& fn,
                     const std::string& info, const Vector<char> data)
// interface function
// Write content data to qar.
{
  TIMER_VERBOSE_FLOPS("write_from_data(QarFile)");
  if (has_regular_file(qar, fn)) {
    qwarn(fname + ssprintf(": qar at '%s' already has fn='%s'. Append anyway.",
                           qar.path.c_str(), fn.c_str()));
  }
  const Long data_len = data.size();
  qassert(data_len >= 0);
  qar_check_if_create_new_vol(qar, data_len);
  const Long total_bytes = write_from_data(qar.back(), fn, info, data);
  timer.flops += total_bytes;
  return total_bytes;
}

Long write_from_data(QarFile& qar, const std::string& fn,
                     const std::string& info, const std::string& data)
{
  TIMER_VERBOSE_FLOPS("write_from_data(QarFile)");
  if (has_regular_file(qar, fn)) {
    qwarn(fname + ssprintf(": qar at '%s' already has fn='%s'. Append anyway.",
                           qar.path.c_str(), fn.c_str()));
  }
  const Long data_len = data.size();
  qassert(data_len >= 0);
  qar_check_if_create_new_vol(qar, data_len);
  const Long total_bytes = write_from_data(qar.back(), fn, info, data);
  timer.flops += total_bytes;
  return total_bytes;
}

Long write_from_data(QarFile& qar, const std::string& fn,
                     const std::string& info,
                     const std::vector<std::string>& data)
{
  TIMER_VERBOSE_FLOPS("write_from_data(QarFile)");
  if (has_regular_file(qar, fn)) {
    qwarn(fname + ssprintf(": qar at '%s' already has fn='%s'. Append anyway.",
                           qar.path.c_str(), fn.c_str()));
  }
  Long data_len = 0;
  for (Long i = 0; i < (Long)data.size(); ++i) {
    data_len += data[i].size();
  }
  qassert(data_len >= 0);
  qar_check_if_create_new_vol(qar, data_len);
  const Long total_bytes = write_from_data(qar.back(), fn, info, data);
  timer.flops += total_bytes;
  return total_bytes;
}

// ----------------------------------------------------

std::vector<std::string> properly_truncate_qar_file(const std::string& path,
                                                    const bool is_only_check)
{
  TIMER_VERBOSE("properly_truncate_qar_file");
  std::vector<std::string> fn_list;
  for (Long iv = 0; iv < 1024 * 1024 * 1024; ++iv) {
    const std::string path_qar_v = path + qar_file_multi_vol_suffix(iv);
    if (not does_file_exist_cache(path_qar_v)) {
      break;
    }
    vector_append(fn_list,
                  properly_truncate_qar_vol_file(path_qar_v, is_only_check));
  }
  return fn_list;
}

// ----------------------------------------------------

std::vector<std::string> show_qar_index(const QarFile& qar)
// interface function
{
  TIMER("show_qar_index");
  std::vector<std::string> lines;
  if (qar.null()) {
    return lines;
  }
  lines.push_back(qar_idx_header);
  for (Long i = 0; i < (Long)qar.size(); ++i) {
    const QarFileVol& qar_v = qar[i];
    qassert(not qar_v.null());
    if (qar_v.mode() == QFileMode::Read) {
      read_through(qar_v);
    }
    const std::vector<std::string>& fn_list = qar_v.p->fn_list;
    const std::map<std::string, QarSegmentInfo> qsinfo_map =
        qar_v.p->qsinfo_map;
    for (Long j = 0; j < (Long)fn_list.size(); ++j) {
      const std::string& fn = fn_list[j];
      const QarSegmentInfo& qsinfo = qsinfo_map.at(fn);
      qassert(qsinfo.fn_len == (Long)fn.size());
      const std::string line1 =
          ssprintf("QAR-FILE-IDX %ld %ld %ld\n", i, j, fn.size());
      const std::string line2 = fn + "\n";
      const std::string line3 = ssprintf(
          "%ld %ld %ld %ld %ld %ld %ld %ld\n", qsinfo.offset, qsinfo.offset_fn,
          qsinfo.offset_info, qsinfo.offset_data, qsinfo.offset_end,
          qsinfo.fn_len, qsinfo.info_len, qsinfo.data_len);
      lines.push_back(line1);
      lines.push_back(line2);
      lines.push_back(line3);
      lines.push_back("\n");
    }
  }
  return lines;
}

int save_qar_index(const QarFile& qar, const std::string& fn)
// interface function
{
  TIMER("save_qar_index");
  std::vector<std::string> lines = show_qar_index(qar);
  if (lines.size() == 0) {
    return 1;
  }
  return qtouch(fn, lines);
}

int parse_qar_index(std::vector<Long>& vol_idx_vec,
                    std::vector<std::string>& fn_vec,
                    std::vector<QarSegmentInfo>& qsinfo_vec,
                    const std::string& qar_index_content)
// interface function
{
  TIMER("parse_qar_index(vol_idx_vec,fn_vec,qsinfo_vec,qar_index_content)");
  vol_idx_vec.clear();
  fn_vec.clear();
  qsinfo_vec.clear();
  std::vector<Long> idx_vec;
  if (qar_index_content == "") {
    return 2;
  }
  const Long header_len = qar_idx_header.size();
  if (0 != qar_index_content.compare(0, header_len, qar_idx_header)) {
    qwarn(fname + ": not qar-idx file format.");
    return 3;
  }
  Long cur = header_len;
  while (cur < (Long)qar_index_content.size()) {
    Long i, j, fn_len;
    std::string fn;
    QarSegmentInfo qsinfo;
    Long cur1 = 0;
    Long cur3 = 0;
    std::string line1;
    std::string line3;
    // line1: QAR-FILE-IDX i j fn_len
    if (not parse_line(line1, cur, qar_index_content)) {
      qwarn(fname + ": not qar-idx file format.");
      return 4;
    }
    if (not parse_literal(cur1, line1, "QAR-FILE-IDX ")) {
      qwarn(fname + ssprintf(": not qar-idx file format. cur1=%ld line1='%s'.",
                             cur1, line1.c_str()));
      return 5;
    }
    if (not parse_long(i, cur1, line1)) {
      qwarn(fname + ": not qar-idx file format.");
      return 6;
    }
    if (not parse_literal(cur1, line1, ' ')) {
      qwarn(fname + ": not qar-idx file format.");
      return 7;
    }
    if (not parse_long(j, cur1, line1)) {
      qwarn(fname + ": not qar-idx file format.");
      return 8;
    }
    if (not parse_literal(cur1, line1, ' ')) {
      qwarn(fname + ": not qar-idx file format.");
      return 9;
    }
    if (not parse_long(fn_len, cur1, line1)) {
      qwarn(fname + ": not qar-idx file format.");
      return 10;
    }
    if (not parse_literal(cur1, line1, '\n')) {
      qwarn(fname + ssprintf(": not qar-idx file format. cur1=%ld line1='%s'.",
                             cur1, line1.c_str()));
      return 11;
    }
    if (not parse_end(cur1, line1)) {
      qwarn(fname + ": not qar-idx file format.");
      return 12;
    }
    // line2: fn
    if (not parse_len(fn, cur, qar_index_content, fn_len)) {
      qwarn(fname + ": not qar-idx file format.");
      return 13;
    }
    if (not parse_literal(cur, qar_index_content, '\n')) {
      qwarn(fname + ": not qar-idx file format.");
      return 14;
    }
    // line3: offset offset_fn offset_info offset_data offset_end fn_len
    // info_len data_len
    if (not parse_line(line3, cur, qar_index_content)) {
      qwarn(fname + ": not qar-idx file format.");
      return 15;
    }
    if (not parse_long(qsinfo.offset, cur3, line3)) {
      qwarn(fname + ": not qar-idx file format.");
      return 16;
    }
    if (not parse_literal(cur3, line3, ' ')) {
      qwarn(fname + ": not qar-idx file format.");
      return 17;
    }
    if (not parse_long(qsinfo.offset_fn, cur3, line3)) {
      qwarn(fname + ": not qar-idx file format.");
      return 18;
    }
    if (not parse_literal(cur3, line3, ' ')) {
      qwarn(fname + ": not qar-idx file format.");
      return 19;
    }
    if (not parse_long(qsinfo.offset_info, cur3, line3)) {
      qwarn(fname + ": not qar-idx file format.");
      return 20;
    }
    if (not parse_literal(cur3, line3, ' ')) {
      qwarn(fname + ": not qar-idx file format.");
      return 21;
    }
    if (not parse_long(qsinfo.offset_data, cur3, line3)) {
      qwarn(fname + ": not qar-idx file format.");
      return 22;
    }
    if (not parse_literal(cur3, line3, ' ')) {
      qwarn(fname + ": not qar-idx file format.");
      return 23;
    }
    if (not parse_long(qsinfo.offset_end, cur3, line3)) {
      qwarn(fname + ": not qar-idx file format.");
      return 24;
    }
    if (not parse_literal(cur3, line3, ' ')) {
      qwarn(fname + ": not qar-idx file format.");
      return 25;
    }
    if (not parse_long(qsinfo.fn_len, cur3, line3)) {
      qwarn(fname + ": not qar-idx file format.");
      return 26;
    }
    if (not parse_literal(cur3, line3, ' ')) {
      qwarn(fname + ": not qar-idx file format.");
      return 27;
    }
    if (not parse_long(qsinfo.info_len, cur3, line3)) {
      qwarn(fname + ": not qar-idx file format.");
      return 28;
    }
    if (not parse_literal(cur3, line3, ' ')) {
      qwarn(fname + ": not qar-idx file format.");
      return 29;
    }
    if (not parse_long(qsinfo.data_len, cur3, line3)) {
      qwarn(fname + ": not qar-idx file format.");
      return 30;
    }
    if (not parse_literal(cur3, line3, '\n')) {
      qwarn(fname + ": not qar-idx file format.");
      return 31;
    }
    if (not parse_end(cur3, line3)) {
      qwarn(fname + ": not qar-idx file format.");
      return 32;
    }
    // line4:
    if (not parse_literal(cur, qar_index_content, '\n')) {
      qwarn(fname + ": not qar-idx file format.");
      return 33;
    }
    if (not qsinfo.check_offset()) {
      return 34;
    }
    if (i < 0) {
      return 35;
    }
    vol_idx_vec.push_back(i);
    idx_vec.push_back(j);
    fn_vec.push_back(fn);
    qsinfo_vec.push_back(qsinfo);
  }
  qassert(vol_idx_vec.size() == idx_vec.size());
  qassert(fn_vec.size() == idx_vec.size());
  qassert(qsinfo_vec.size() == idx_vec.size());
  Long vol_idx_current = 0;
  Long idx_current = 0;
  for (Long k = 0; k < (Long)idx_vec.size(); ++k) {
    const Long i = vol_idx_vec[k];
    const Long j = idx_vec[k];
    if (i != vol_idx_current) {
      if (idx_current <= 0) {
        return 36;
      }
      vol_idx_current += 1;
      idx_current = 0;
    }
    if (vol_idx_current != i) {
      return 37;
    }
    if (idx_current != j) {
      return 38;
    }
    idx_current += 1;
  }
  return 0;
}

int parse_qar_index(const QarFile& qar, const std::string& qar_index_content)
// interface function
{
  TIMER("parse_qar_index(qar,qar_index_content)");
  std::vector<Long> vol_idx_vec;
  std::vector<std::string> fn_vec;
  std::vector<QarSegmentInfo> qsinfo_vec;
  if (qar.null()) {
    qwarn(fname + ": qar is null.");
    return 1;
  }
  const int ret =
      parse_qar_index(vol_idx_vec, fn_vec, qsinfo_vec, qar_index_content);
  if (ret != 0) {
    qwarn(fname +
          ssprintf(": index is not correct for '%s'.", qar.path.c_str()));
    return ret;
  }
  qassert(fn_vec.size() == vol_idx_vec.size());
  qassert(qsinfo_vec.size() == vol_idx_vec.size());
  for (Long k = 0; k < (Long)vol_idx_vec.size(); ++k) {
    const Long i = vol_idx_vec[k];
    const std::string& fn = fn_vec[k];
    const QarSegmentInfo& qsinfo = qsinfo_vec[k];
    register_file(qar[i], fn, qsinfo);
  }
  return 0;
}

int load_qar_index(const QarFile& qar, const std::string& fn)
// interface function
{
  TIMER_VERBOSE("load_qar_index");
  const std::string qar_index_content = qcat(fn);
  if (qar_index_content == "") {
    return 1;
  } else {
    return parse_qar_index(qar, qar_index_content);
  }
}

// ----------------------------------------------------

std::string mk_key_from_qar_path(const std::string& path)
{
  if (path.size() <= 4) {
    return "";
  }
  if (0 != path.compare(path.size() - 4, 4, ".qar")) {
    return "";
  }
  const std::string key = path.substr(0, path.size() - 4) + "/";
  return key;
}

std::string mk_new_qar_read_cache_key(const QarFile& qar,
                                      const std::string& key,
                                      const std::string& path)
// (1) Find the first new qar file in qar that match the prefix of path and
// register the new qar file in qar_read_cache.
// (2) If qar not found, return key.
// (3) If path exists in the qar, return the new key of the new qar.
// (4) If not found, repeat the procedure for the new qar.
{
  Cache<std::string, QarFile>& cache = get_qar_read_cache();
  std::string path_dir = remove_trailing_slashes(path);
  const std::string pathd = path_dir + "/";
  while (true) {
    if (path_dir == "/" or path_dir == ".") {
      return key;
    }
    if (has_regular_file(qar, path_dir + ".qar")) {
      const std::string key_new = path_dir + "/";
      qassert(pathd.substr(0, key_new.size()) == key_new);
      QarFile& qar_new = cache[key + key_new];
      if (qar_new.null()) {
        qar_new.init(key + path_dir + ".qar", QFileMode::Read);
      }
      qassert(not qar_new.null());
      const std::string path_new = pathd.substr(key_new.size());
      if (has(qar_new, path_new)) {
        return key + key_new;
      } else {
        return mk_new_qar_read_cache_key(qar_new, key + key_new, path_new);
      }
    }
    path_dir = dirname(path_dir);
  }
  qassert(false);
  return "";
}

std::string mk_new_qar_read_cache_key(const std::string& path)
// (1) Find first qar file that match the prefix of path and register the qar
// file in qar_read_cache.
// (2) If qar not found, return "".
// (2) If path exists in the qar, return the key of qar.
// (4) If not found, find qar within qar recursively, return the key of the
// closest qar.
{
  Cache<std::string, QarFile>& cache = get_qar_read_cache();
  std::string path_dir = remove_trailing_slashes(path);
  const std::string pathd = path_dir + "/";
  while (true) {
    if (path_dir == "/" or path_dir == ".") {
      return "";
    }
    if (does_file_exist(path_dir + ".qar")) {
      const std::string key = path_dir + "/";
      qassert(pathd.substr(0, key.size()) == key);
      qassert(not cache.has(key));
      QarFile& qar = cache[key];
      qar.init(path_dir + ".qar", QFileMode::Read);
      qassert(not qar.null());
      const std::string path_new = pathd.substr(key.size());
      if (has(qar, path_new)) {
        return key;
      } else {
        return mk_new_qar_read_cache_key(qar, key, path_new);
      }
    }
    path_dir = dirname(path_dir);
  }
  qassert(false);
  return "";
}

std::string get_qar_read_cache_key(const std::string& path)
// return key of get_qar_read_cache() that may contain path
// return empty string if no cached key is found.
// Note: key should end with '/'.
// Steps:
// (1) Check if path exists with does_file_exist_cache. If exists, return path.
// (2) If not found, search in Qar Cache. If found matching key, try to find
// within this qar file recursively. Return the key of the closest match.
// (3) If does not exist, try to find qar file yet to be in cache recursively.
// Return values: valid key: valid key for a qar found. (qar may not actually
// contain path).
// "": no key is found and path does not exist.
// path: path exist.
{
  TIMER("get_qar_read_cache_key");
  if (does_file_exist_cache(path)) {
    return path;
  }
  Cache<std::string, QarFile>& cache = get_qar_read_cache();
  for (auto it = cache.m.cbegin(); it != cache.m.cend(); ++it) {
    const std::string& key = it->first;
    if (key == path.substr(0, key.size())) {
      const QarFile& qar = cache[key];
      const std::string path_new = path.substr(key.size());
      if (has(qar, path_new)) {
        return key;
      } else {
        return mk_new_qar_read_cache_key(qar, key, path_new);
      }
    }
  }
  return mk_new_qar_read_cache_key(path);
}

// ----------------------------------------------------

int qar_build_index(const std::string& path_qar)
{
  TIMER_VERBOSE("qar_build_index");
  displayln(fname + ssprintf(": '%s'.", path_qar.c_str()));
  QarFile qar(path_qar, QFileMode::Read);
  const int ret = save_qar_index(qar, path_qar + ".idx");
  qar.close();
  return ret;
}

int qar_create(const std::string& path_qar, const std::string& path_folder_,
               const bool is_remove_folder_after)
// interface function
// return 0 if successful.
{
  TIMER_VERBOSE_FLOPS("qar_create");
  const std::string path_folder = remove_trailing_slashes(path_folder_);
  displayln(
      0, fname + ssprintf(
                     ": '%s' '%s' %s.", path_qar.c_str(), path_folder.c_str(),
                     is_remove_folder_after ? "remove folder" : "keep folder"));
  if (not is_directory(path_folder)) {
    qwarn(fname + ssprintf(": '%s' '%s' no folder.", path_qar.c_str(),
                           path_folder.c_str()));
    return 1;
  }
  if (does_file_exist(path_qar)) {
    qwarn(fname + ssprintf(": '%s' '%s' qar already exist.", path_qar.c_str(),
                           path_folder.c_str()));
    return 2;
  }
  if (does_file_exist(path_qar + ".acc")) {
    qwarn(fname + ssprintf(": '%s' '%s' qar.acc already exist.",
                           path_qar.c_str(), path_folder.c_str()));
    return 3;
  }
  const std::string path_prefix = path_folder + "/";
  const Long path_prefix_len = path_prefix.size();  // including the final "/"
  const std::vector<std::string> contents = qls_all(path_folder);
  std::vector<std::string> reg_files;
  for (Long i = 0; i < (Long)contents.size(); ++i) {
    const std::string path = contents[i];
    qassert(path.substr(0, path_prefix_len) == path_prefix);
    if (not is_directory(path)) {
      if (not is_regular_file(path)) {
        qwarn(fname + ssprintf(": '%s' '%s' '%s' not regular file.",
                               path_qar.c_str(), path_folder.c_str(),
                               path.c_str()));
        return 4;
      }
      reg_files.push_back(path);
    }
  }
  QarFile qar(path_qar + ".acc", QFileMode::Append);
  for (Long i = 0; i < (Long)reg_files.size(); ++i) {
    const std::string path = reg_files[i];
    const std::string fn = path.substr(path_prefix_len);
    QFile qfile_in(path, QFileMode::Read);
    qassert(not qfile_in.null());
    write_from_qfile(qar, fn, "", qfile_in);
    qfile_in.close();
  }
  const Long num_vol = qar.size();
  save_qar_index(qar, path_qar + ".acc.idx");
  qar.close();
  int ret_rename = 0;
  for (Long iv = 0; iv < num_vol; ++iv) {
    const std::string path_qar_v_acc =
        path_qar + ".acc" + qar_file_multi_vol_suffix(iv);
    const std::string path_qar_v = path_qar + qar_file_multi_vol_suffix(iv);
    ret_rename = qrename(path_qar_v_acc, path_qar_v);
    qassert(ret_rename == 0);
  }
  ret_rename = qrename(path_qar + ".acc.idx", path_qar + ".idx");
  qassert(ret_rename == 0);
  qar.init(path_qar, QFileMode::Read);
  qassert((Long)qar.size() == num_vol);
  if (not verify_index(qar)) {
    qerr(fname + ": idx verification failed.");
  }
  if (is_remove_folder_after) {
    for (Long iv = 0; iv < num_vol; ++iv) {
      const std::string path_qar_v = path_qar + qar_file_multi_vol_suffix(iv);
      qassert(does_file_exist(path_qar_v));
    }
    qremove_all(path_folder);
  }
  return 0;
}

int qar_extract(const std::string& path_qar, const std::string& path_folder_,
                const bool is_remove_qar_after)
// interface function
// return 0 if successful.
{
  TIMER_VERBOSE_FLOPS("qar_extract");
  const std::string path_folder = remove_trailing_slashes(path_folder_);
  displayln(
      0,
      fname + ssprintf(": '%s' '%s' %s.", path_qar.c_str(), path_folder.c_str(),
                       is_remove_qar_after ? "remove qar " : "keep qar"));
  if (not does_regular_file_exist_qar(path_qar)) {
    qwarn(fname + ssprintf(": '%s' '%s' qar does not exist.", path_qar.c_str(),
                           path_folder.c_str()));
    return 1;
  }
  if (does_file_exist(path_folder)) {
    qwarn(fname + ssprintf(": '%s' '%s' folder exist.", path_qar.c_str(),
                           path_folder.c_str()));
    return 2;
  }
  if (does_file_exist(path_folder + ".acc")) {
    qwarn(fname + ssprintf(": '%s' '%s' folder.acc already exist.",
                           path_qar.c_str(), path_folder.c_str()));
    return 3;
  }
  QarFile qar(path_qar, QFileMode::Read);
  const std::vector<std::string> contents = list(qar);
  std::set<std::string> dirs;
  qmkdir_p(path_folder + ".acc");
  dirs.insert(".");
  for (Long i = 0; i < (Long)contents.size(); ++i) {
    const std::string& fn = contents[i];
    const std::string dn = dirname(fn);
    if (not has(dirs, dn)) {
      const int code = qmkdir_p(path_folder + ".acc/" + dn);
      qassert(code == 0);
      dirs.insert(dn);
    }
    QFile qfile_in = read(qar, fn);
    qassert(not qfile_in.null());
    QFile qfile_out(path_folder + ".acc/" + fn, QFileMode::Write);
    qassert(not qfile_out.null());
    timer.flops += write_from_qfile(qfile_out, qfile_in);
    qfclose(qfile_in);
    qfclose(qfile_out);
  }
  const Long num_vol = qar.size();
  qar.close();
  qrename(path_folder + ".acc", path_folder);
  if (is_remove_qar_after) {
    qassert(is_directory(path_folder));
    if (does_file_exist(path_qar + ".idx")) {
      qremove(path_qar + ".idx");
    }
    for (Long iv = 0; iv < num_vol; ++iv) {
      const std::string path_qar_v = path_qar + qar_file_multi_vol_suffix(iv);
      if (does_file_exist(path_qar_v)) {
        qremove(path_qar_v);
      }
    }
  }
  return 0;
}

int qcopy_file(const std::string& path_src, const std::string& path_dst)
// interface function
// return 0 if successful.
{
  TIMER_VERBOSE_FLOPS("qcopy_file");
  displayln(0,
            fname + ssprintf(": '%s' %s.", path_src.c_str(), path_dst.c_str()));
  if (not does_regular_file_exist_qar(path_src)) {
    qwarn(fname + ssprintf(": '%s' does not exist.", path_src.c_str()));
    return 1;
  }
  if (does_file_exist(path_dst)) {
    qwarn(fname + ssprintf(": '%s' already exist.", path_dst.c_str()));
    return 2;
  }
  if (does_file_exist(path_dst + ".acc")) {
    qwarn(fname +
          ssprintf(": '%s' already exist.", (path_dst + ".acc").c_str()));
    return 3;
  }
  QFile qfile_in = qfopen(path_src, QFileMode::Read);
  qassert(not qfile_in.null());
  QFile qfile_out = qfopen(path_dst + ".acc", QFileMode::Write);
  qassert(not qfile_out.null());
  timer.flops += write_from_qfile(qfile_out, qfile_in);
  qfclose(qfile_out);
  qfclose(qfile_in);
  qrename(path_dst + ".acc", path_dst);
  return 0;
}

// ----------------------------------------------------

bool does_regular_file_exist_qar(const std::string& path)
// interface function
// Note: should only check file, not directory.
{
  TIMER("does_regular_file_exist_qar");
  const std::string key = get_qar_read_cache_key(path);
  if (key == path) {
    return true;
  } else if (key == "") {
    return false;
  }
  qassert(key == path.substr(0, key.size()));
  const std::string fn = path.substr(key.size());
  QarFile& qar = get_qar_read_cache()[key];
  return has_regular_file(qar, fn);
}

bool does_file_exist_qar(const std::string& path)
// interface function
{
  TIMER("does_file_exist_qar");
  const std::string key = get_qar_read_cache_key(path);
  if (key == path) {
    return true;
  } else if (key == "") {
    return false;
  }
  qassert(key == path.substr(0, key.size()));
  const std::string fn = path.substr(key.size());
  QarFile& qar = get_qar_read_cache()[key];
  return has(qar, fn);
}

std::vector<std::string> list_qar(const std::string& path)
{
  if (not does_file_exist_qar(path)) {
    std::vector<std::string> ret;
    return ret;
  }
  const std::string key = mk_key_from_qar_path(path);
  if (key != "") {
    Cache<std::string, QarFile>& cache = get_qar_read_cache();
    QarFile& qar = cache[key];
    if (qar.null()) {
      qar.init(path, QFileMode::Read);
    }
    return list(qar);
  } else {
    QarFile qar(path, QFileMode::Read);
    return list(qar);
  }
}

std::string qcat(const std::string& path)
{
  TIMER("qcat(fn)");
  QFile qfile = qfopen(path, QFileMode::Read);
  qassert(not qfile.null());
  const std::string ret = qcat(qfile);
  qfclose(qfile);
  return ret;
}

std::vector<std::string> qgetlines(const std::string& fn)
{
  QFile qfile = qfopen(fn, QFileMode::Read);
  qassert(not qfile.null());
  const std::vector<std::string> ret = qgetlines(qfile);
  qfclose(qfile);
  return ret;
}

int qtouch(const std::string& path)
// return 0 if success
{
  TIMER("qtouch(fn)");
  QFile qfile = qfopen(path, QFileMode::Write);
  if (qfile.null()) {
    return 1;
  }
  qfclose(qfile);
  return 0;
}

int qtouch(const std::string& path, const std::string& content)
{
  TIMER("qtouch(fn,content)");
  QFile qfile = qfopen(path + ".partial", QFileMode::Write);
  if (qfile.null()) {
    return 1;
  }
  const int ret = qappend(qfile, content);
  qfclose(qfile);
  if (ret != 0) {
    return ret;
  }
  return qrename(path + ".partial", path);
}

int qtouch(const std::string& path, const std::vector<std::string>& content)
{
  TIMER("qtouch(fn,content)");
  QFile qfile = qfopen(path + ".partial", QFileMode::Write);
  if (qfile.null()) {
    return 1;
  }
  const int ret = qappend(qfile, content);
  qfclose(qfile);
  if (ret != 0) {
    return ret;
  }
  return qrename(path + ".partial", path);
}

int qappend(const std::string& path, const std::string& content)
{
  TIMER("qappend(fn,content)");
  QFile qfile = qfopen(path, QFileMode::Append);
  if (qfile.null()) {
    return 1;
  }
  const int ret = qappend(qfile, content);
  qfclose(qfile);
  return ret;
}

int qappend(const std::string& path, const std::vector<std::string>& content)
{
  TIMER("qappend(fn,content)");
  QFile qfile = qfopen(path, QFileMode::Append);
  if (qfile.null()) {
    return 1;
  }
  const int ret = qappend(qfile, content);
  qfclose(qfile);
  return ret;
}

DataTable qload_datatable_serial(QFile& qfile)
{
  TIMER("qload_datatable_serial(qfile)");
  DataTable ret;
  while (not qfeof(qfile)) {
    const std::string line = qgetline(qfile);
    if (line.length() > 0 && line[0] != '#') {
      const std::vector<double> xs = read_doubles(line);
      if (xs.size() > 0) {
        ret.push_back(xs);
      }
    }
  }
  return ret;
}

DataTable qload_datatable_par(QFile& qfile)
{
  TIMER("qload_datatable_qar(qfile)");
  const size_t line_buf_size = 1024;
  DataTable ret;
  std::vector<std::string> lines;
  DataTable xss;
  while (not qfeof(qfile)) {
    lines.clear();
    for (size_t i = 0; i < line_buf_size; ++i) {
      lines.push_back(qgetline(qfile));
      if (qfeof(qfile)) {
        break;
      }
    }
    xss.resize(lines.size());
#pragma omp parallel for
    for (size_t i = 0; i < lines.size(); ++i) {
      const std::string& line = lines[i];
      if (line.length() > 0 && line[0] != '#') {
        xss[i] = read_doubles(line);
      } else {
        clear(xss[i]);
      }
    }
    for (size_t i = 0; i < xss.size(); ++i) {
      if (xss[i].size() > 0) {
        ret.push_back(xss[i]);
      }
    }
  }
  return ret;
}

DataTable qload_datatable_serial(const std::string& path)
{
  TIMER("qload_datatable_serial(path)");
  if (not does_regular_file_exist_qar(path)) {
    return DataTable();
  }
  QFile qfile = qfopen(path, QFileMode::Read);
  qassert(not qfile.null());
  DataTable ret = qload_datatable_serial(qfile);
  qfclose(qfile);
  return ret;
}

DataTable qload_datatable_par(const std::string& path)
{
  TIMER("qload_datatable_par(path)");
  if (not does_regular_file_exist_qar(path)) {
    return DataTable();
  }
  QFile qfile = qfopen(path, QFileMode::Read);
  qassert(not qfile.null());
  DataTable ret = qload_datatable_par(qfile);
  qfclose(qfile);
  return ret;
}

DataTable qload_datatable(const std::string& path, const bool is_par)
{
  if (is_par) {
    return qload_datatable_par(path);
  } else {
    return qload_datatable_serial(path);
  }
}

// -------------------

crc32_t compute_crc32(QFile& qfile)
// interface function
// compute_crc32 for all data.
{
  TIMER_VERBOSE_FLOPS("compute_crc32");
  qassert(not qfile.null());
  qassert(qfile.mode() == QFileMode::Read);
  qfseek(qfile, 0, SEEK_SET);
  const size_t chunk_size = 16 * 1024 * 1024;
  std::vector<char> data(chunk_size);
  crc32_t crc = 0;
  while (true) {
    const Long size = qread_data(get_data(data), qfile);
    timer.flops += size;
    if (size == 0) {
      break;
    }
    crc = crc32_par(crc, Vector<char>(data.data(), size));
  }
  return crc;
}

crc32_t compute_crc32(const std::string& path)
{
  QFile qfile = qfopen(path, QFileMode::Read);
  const crc32_t ret = compute_crc32(qfile);
  qfclose(qfile);
  return ret;
}

void check_all_files_crc32_aux(
    std::vector<std::pair<std::string, crc32_t>>& acc, const std::string& path)
{
  if (not is_directory(path)) {
    acc.push_back(check_file_crc32(path));
  } else {
    const std::vector<std::string> paths = qls(path);
    for (Long i = 0; i < (Long)paths.size(); ++i) {
      check_all_files_crc32_aux(acc, paths[i]);
    }
  }
}

std::vector<std::pair<std::string, crc32_t>> check_all_files_crc32(
    const std::string& path)
{
  TIMER_VERBOSE("check_all_files_crc32");
  std::vector<std::pair<std::string, crc32_t>> ret;
  check_all_files_crc32_aux(ret, remove_trailing_slashes(path));
  return ret;
}

void check_all_files_crc32_info(const std::string& path)
// interface function
{
  TIMER_VERBOSE("check_all_files_crc32_info");
  if (0 == get_id_node()) {
    displayln(fname + ssprintf(": start checking path='%s'", path.c_str()));
    std::vector<std::pair<std::string, crc32_t>> fcrcs;
    fcrcs = check_all_files_crc32(path);
    displayln(fname + ssprintf(": summary for path='%s'", path.c_str()));
    display(show_files_crc32(fcrcs));
  }
}

std::string show_file_crc32(const std::pair<std::string, crc32_t>& fcrc)
{
  return ssprintf("%08X  fn='%s'", fcrc.second, fcrc.first.c_str());
}

std::string show_files_crc32(
    const std::vector<std::pair<std::string, crc32_t>>& fcrcs)
{
  std::ostringstream out;
  for (Long i = 0; i < (Long)fcrcs.size(); ++i) {
    out << ssprintf("%5ld ", i) << show_file_crc32(fcrcs[i]) << std::endl;
  }
  return out.str();
}

std::pair<std::string, crc32_t> check_file_crc32(const std::string& fn)
{
  TIMER_VERBOSE("check_file_crc32");
  std::pair<std::string, crc32_t> p;
  p.first = fn;
  p.second = compute_crc32(fn);
  displayln_info(show_file_crc32(p));
  return p;
}

// -------------------

int qar_build_index_info(const std::string& path_qar)
{
  TIMER_VERBOSE("qar_build_index_info")
  if (0 == get_id_node()) {
    return qar_build_index(path_qar);
  } else {
    remove_entry_directory_cache(path_qar + ".idx");
    return 0;
  }
}

int qar_create_info(const std::string& path_qar,
                    const std::string& path_folder_,
                    const bool is_remove_folder_after)
{
  if (0 == get_id_node()) {
    return qar_create(path_qar, path_folder_, is_remove_folder_after);
  } else {
    if (is_remove_folder_after) {
      remove_entry_directory_cache(path_folder_);
    }
    return 0;
  }
}

int qar_extract_info(const std::string& path_qar,
                     const std::string& path_folder_,
                     const bool is_remove_qar_after)
{
  if (0 == get_id_node()) {
    return qar_extract(path_qar, path_folder_, is_remove_qar_after);
  } else {
    remove_entry_directory_cache(path_folder_);
    return 0;
  }
}

int qcopy_file_info(const std::string& path_src, const std::string& path_dst)
{
  Long ret = 0;
  if (0 == get_id_node()) {
    ret = qcopy_file(path_src, path_dst);
  } else {
    const std::string path_dir = dirname(path_dst);
    remove_entry_directory_cache(path_dir);
    return 0;
  }
  return ret;
}

std::string qcat_info(const std::string& path)
{
  TIMER("qcat_info(fn)");
  if (0 == get_id_node()) {
    return qcat(path);
  } else {
    return std::string();
  }
}

int qtouch_info(const std::string& path)
{
  TIMER("qtouch_info");
  if (0 == get_id_node()) {
    return qtouch(path);
  } else {
    return 0;
  }
}

int qtouch_info(const std::string& path, const std::string& content)
{
  TIMER("qtouch_info");
  if (0 == get_id_node()) {
    return qtouch(path, content);
  } else {
    return 0;
  }
}

int qtouch_info(const std::string& path,
                const std::vector<std::string>& content)
{
  TIMER("qtouch_info");
  if (0 == get_id_node()) {
    return qtouch(path, content);
  } else {
    return 0;
  }
}

int qappend_info(const std::string& path, const std::string& content)
{
  TIMER("qappend_info(fn,content)");
  if (0 == get_id_node()) {
    return qappend(path, content);
  } else {
    return 0;
  }
}

int qappend_info(const std::string& path, const std::vector<std::string>& content)
{
  TIMER("qappend_info(fn,content)");
  if (0 == get_id_node()) {
    return qappend(path, content);
  } else {
    return 0;
  }
}

// ---------------------------------

int qar_create_sync_node(const std::string& path_qar,
                         const std::string& path_folder_,
                         const bool is_remove_folder_after)
{
  Long ret = 0;
  if (0 == get_id_node()) {
    ret = qar_create(path_qar, path_folder_, is_remove_folder_after);
  } else {
    if (is_remove_folder_after) {
      remove_entry_directory_cache(path_folder_);
    }
  }
  glb_sum_long(ret);
  return ret;
}

int qar_extract_sync_node(const std::string& path_qar,
                          const std::string& path_folder_,
                          const bool is_remove_qar_after)
{
  Long ret = 0;
  if (0 == get_id_node()) {
    ret = qar_extract(path_qar, path_folder_, is_remove_qar_after);
  } else {
    remove_entry_directory_cache(path_folder_);
  }
  glb_sum_long(ret);
  return ret;
}

int qcopy_file_sync_node(const std::string& path_src,
                         const std::string& path_dst)
{
  Long ret = 0;
  if (0 == get_id_node()) {
    ret = qcopy_file(path_src, path_dst);
  } else {
    const std::string path_dir = dirname(path_dst);
    remove_entry_directory_cache(path_dir);
  }
  glb_sum_long(ret);
  return ret;
}

bool does_regular_file_exist_qar_sync_node(const std::string& fn)
{
  Long nfile = 0;
  if (0 == get_id_node()) {
    if (does_regular_file_exist_qar(fn)) {
      nfile = 1;
    }
  }
  glb_sum_long(nfile);
  return 0 != nfile;
}

bool does_file_exist_qar_sync_node(const std::string& fn)
{
  Long nfile = 0;
  if (0 == get_id_node()) {
    if (does_file_exist_qar(fn)) {
      nfile = 1;
    }
  }
  glb_sum_long(nfile);
  return 0 != nfile;
}

std::string qcat_sync_node(const std::string& path)
{
  TIMER("qcat_sync_node");
  std::string ret;
  if (0 == get_id_node()) {
    ret = qcat(path);
  }
  int bret = bcast_with_glb_sum(ret);
  qassert(bret == 0);
  return ret;
}

DataTable qload_datatable_sync_node(const std::string& path, const bool is_par)
{
  TIMER_VERBOSE("qload_datatable_sync_node");
  DataTable dt;
  if (0 == get_id_node()) {
    dt = qload_datatable(path, is_par);
  }
  int bret = bcast_with_glb_sum(dt);
  qassert(bret == 0);
  return dt;
}

}  // namespace qlat
