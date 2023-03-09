#pragma once

#include <qlat-utils/env.h>
#include <qlat-utils/cache.h>
#include <qlat-utils/crc32.h>
#include <qlat-utils/qar.h>
#include <qlat-utils/types.h>

namespace qlat
{  //

bool does_regular_file_exist_qar(const std::string& path);

bool does_file_exist_qar(const std::string& path);

QFile qfopen(const std::string& path, const std::string& mode);

std::string qcat(const std::string& path);

int qar_create(const std::string& path_qar, const std::string& path_folder_,
               const bool is_remove_folder_after = false);

int qar_extract(const std::string& path_qar, const std::string& path_folder_,
                const bool is_remove_qar_after = false);

int qcopy_file(const std::string& path_src, const std::string& path_dst);

std::vector<std::string> list_qar(const std::string& path);

DataTable qload_datatable(const std::string& path, const bool is_par = false);

int qtouch(const std::string& path);

int qtouch(const std::string& path, const std::string& content);

int qtouch(const std::string& path, const std::vector<std::string>& content);

int qappend(const std::string& path, const std::string& content);

crc32_t compute_crc32(const std::string& path);

std::vector<std::pair<std::string, crc32_t> > check_all_files_crc32(
    const std::string& path);

void check_all_files_crc32_info(const std::string& path);

// -------------------

inline std::string qar_file_multi_vol_suffix(const long i)
{
  if (i == 0) {
    return "";
  } else {
    return ssprintf(".v%ld", i);
  }
  qassert(false);
  return "";
}

struct API QarFile : std::vector<QarFileVol> {
  // Only for reading
  QarFile() { init(); }
  QarFile(const std::string& path_qar, const std::string& mode)
  {
    init(path_qar, mode);
  }
  //
  void init()
  {
    std::vector<QarFileVol>& v = *this;
    qlat::clear(v);
  }
  void init(const std::string& path_qar, const std::string& mode)
  {
    init();
    if (mode == "r") {
      // maximally 1024 * 1024 * 1024 volumes
      for (long iv = 0; iv < 1024 * 1024 * 1024; ++iv) {
        const std::string path_qar_v = path_qar + qar_file_multi_vol_suffix(iv);
        if (not does_regular_file_exist_qar(path_qar_v)) {
          break;
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
  //
  void close() { init(); }
  //
  bool null() const { return size() == 0; }
};

std::vector<std::string> list(const QarFile& qar);

bool has_regular_file(const QarFile& qar, const std::string& fn);

bool has(const QarFile& qar, const std::string& fn);

bool read(const QarFile& qar, const std::string& fn, QFile& qfile_in);

// -------------------

API inline Cache<std::string, QarFile>& get_qar_read_cache()
// key should be the path prefix of the contents of the qar file.
// Note: key should end with '/'.
{
  static Cache<std::string, QarFile> cache("QarReadCache", 64, 1);
  return cache;
}

inline std::string mk_new_qar_read_cache_key(const QarFile& qar,
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
        qar_new.init(key + path_dir + ".qar", "r");
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

inline std::string mk_new_qar_read_cache_key(const std::string& path)
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
      qar.init(path_dir + ".qar", "r");
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

inline std::string get_qar_read_cache_key(const std::string& path)
// return key of get_qar_read_cache() that may contain path
// return empty string if no cached key is found.
// Note: key should end with '/'.
// Steps:
// (1) Search in Cache. If found matching key, try to find within this qar file
// recursively. Return the key of the closest match.
// (2) If not found, check if path exists. If exists, return path.
// (3) If does not exist, try to find qar file yet to be in cache recursively.
// Return values:
// valid key: valid key for a qar found. (qar may not actually contain path).
// "": no key is found and path does not exist.
// path: path exist.
{
  TIMER("get_qar_read_cache_key");
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
  if (does_file_exist(path)) {
    return path;
  }
  return mk_new_qar_read_cache_key(path);
}

// -------------------

inline crc32_t compute_crc32(QFile& qfile)
// interface function
// compute_crc32 for all the remaining data.
{
  TIMER_VERBOSE_FLOPS("compute_crc32");
  qassert(not qfile.null());
  const size_t chunk_size = 16 * 1024 * 1024;
  std::vector<char> data(chunk_size);
  crc32_t crc = 0;
  while (true) {
    const long size = qread_data(get_data(data), qfile);
    timer.flops += size;
    if (size == 0) {
      break;
    }
    crc = crc32_par(crc, Vector<char>(data.data(), size));
  }
  return crc;
}

inline std::vector<std::string> qgetlines(const std::string& fn)
{
  QFile qfile = qfopen(fn, "r");
  qassert(not qfile.null());
  std::vector<std::string> lines = qgetlines(qfile);
  qfclose(qfile);
  return lines;
}

// -------------------

inline DataTable qload_datatable_serial(QFile& qfile)
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

inline DataTable qload_datatable_par(QFile& qfile)
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

inline DataTable qload_datatable_serial(const std::string& path)
{
  TIMER("qload_datatable_serial(path)");
  if (not does_regular_file_exist_qar(path)) {
    return DataTable();
  }
  QFile qfile = qfopen(path, "r");
  qassert(not qfile.null());
  DataTable ret = qload_datatable_serial(qfile);
  qfclose(qfile);
  return ret;
}

inline DataTable qload_datatable_par(const std::string& path)
{
  TIMER("qload_datatable_par(path)");
  if (not does_regular_file_exist_qar(path)) {
    return DataTable();
  }
  QFile qfile = qfopen(path, "r");
  qassert(not qfile.null());
  DataTable ret = qload_datatable_par(qfile);
  qfclose(qfile);
  return ret;
}

// -------------------

inline std::string qcat_info(const std::string& path)
{
  TIMER("qcat_info");
  if (0 == get_id_node()) {
    return qcat(path);
  } else {
    return std::string();
  }
}

inline std::string show_file_crc32(const std::pair<std::string, crc32_t>& fcrc)
{
  return ssprintf("%08X  fn='%s'", fcrc.second, fcrc.first.c_str());
}

inline std::string show_files_crc32(
    const std::vector<std::pair<std::string, crc32_t> >& fcrcs)
{
  std::ostringstream out;
  for (long i = 0; i < (long)fcrcs.size(); ++i) {
    out << ssprintf("%5ld ", i) << show_file_crc32(fcrcs[i]) << std::endl;
  }
  return out.str();
}

inline std::pair<std::string, crc32_t> check_file_crc32(const std::string& fn)
{
  TIMER_VERBOSE("check_file_crc32");
  std::pair<std::string, crc32_t> p;
  p.first = fn;
  p.second = compute_crc32(fn);
  displayln_info(show_file_crc32(p));
  return p;
}

inline void check_all_files_crc32_aux(
    std::vector<std::pair<std::string, crc32_t> >& acc, const std::string& path)
{
  if (not is_directory(path)) {
    acc.push_back(check_file_crc32(path));
  } else {
    const std::vector<std::string> paths = qls_aux(path);
    for (long i = 0; i < (long)paths.size(); ++i) {
      check_all_files_crc32_aux(acc, paths[i]);
    }
  }
}

inline int qtouch_info(const std::string& path)
{
  TIMER("qtouch_info");
  if (0 == get_id_node()) {
    return qtouch(path);
  } else {
    return 0;
  }
}

inline int qtouch_info(const std::string& path, const std::string& content)
{
  TIMER("qtouch_info");
  if (0 == get_id_node()) {
    return qtouch(path, content);
  } else {
    return 0;
  }
}

inline int qappend_info(const std::string& path, const std::string& content)
{
  TIMER("qappend_info");
  if (0 == get_id_node()) {
    return qappend(path, content);
  } else {
    return 0;
  }
}

}  // namespace qlat
