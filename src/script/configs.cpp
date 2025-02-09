#include <string.h>

#include "configs.hpp"
#include "options.hpp"

static std::streamsize get_flength(std::ifstream& file) {
  if (!file.is_open()) {
    return 0;
  }
  std::streampos temp_1 = file.tellg();
  file.seekg(0, std::fstream::end);
  std::streampos temp_2 = file.tellg();
  file.seekg(temp_1, std::fstream::beg);

  return temp_2;
}

static std::string read(std::string path) {
  std::ifstream ifile;
  ifile.open(path, std::ios::binary);
  std::streamsize len = get_flength(ifile);
  char* dummy = new char[len];

  try {
    ifile.read(dummy, len);
  } catch (std::exception& err) {
    ifile.close();
    return "";
  }
  if (dummy == nullptr || strlen(dummy) == 0) {
    ifile.close();
    return "";
  }
  ifile.close();
  // dummy += '\0';
  std::string re;
  re.assign(dummy, len);

  delete[] dummy;
  dummy = nullptr;

  return re;
}

void make_file(std::string name, std::string stdc) {
  std::ofstream of;
  of.open(name, std::ios::trunc);
  of.close();
  if (stdc != "") {
    of.open(name, std::ios::app);
    of << stdc;
    of.close();
  }
}

void reset_localconf() {
  if (std::filesystem::exists(CATCARE_HOME)) {
    delete_localconf();
  }

  std::filesystem::create_directory(CATCARE_HOME);
  std::ofstream of;
  make_file(CATCARE_CONFIGFILE);
  of.open(CATCARE_CONFIGFILE, std::ios::app);
  of << "options = {}\nredirects = {}\nblacklist = []\n";
  of.close();
}

void load_localconf() {
  IniFile file = IniFile::from_file(CATCARE_CONFIGFILE);
  IniDictionary s = file.get("options");
  for (auto i : s) {
    options[i.first] = (std::string)i.second;
  }
  IniDictionary z = file.get("redirects");
  for (auto i : z) {
    local_redirect[i.first] = (std::string)i.second;
  }
}

void delete_localconf() { std::filesystem::remove_all(CATCARE_HOME); }

void write_localconf() {
  IniFile file = IniFile::from_file(CATCARE_CONFIGFILE);
  IniDictionary s = file.get("options");
  for (auto i : options) {
    IniElement elm;
    elm = i.second;
    s[i.first] = elm;
  }
  file.set("options", s);
  IniDictionary z = file.get("redirects");
  for (auto i : local_redirect) {
    IniElement elm;
    elm = i.second;
    z[i.first] = elm;
  }
  file.set("redirects", z);
  file.to_file(CATCARE_CONFIGFILE);
}

IniDictionary extract_configs(std::string name) {
  IniDictionary ret;

  IniFile file = IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH + name +
                                    CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
  if (!file) {
    return ret;
  }

  if (!file || !file.has("name", "Info") || !file.has("files", "Download")) {
    return ret;
  }

  ret["name"] = file.get("name", "Info");
  ret["files"] = file.get("files", "Download");
  if (file.has("version", "Info")) ret["version"] = file.get("version", "Info");
  if (file.has("dependencies", "Download"))
    ret["dependencies"] = file.get("dependencies", "Download");
  if (file.has("scripts", "Download"))
    ret["scripts"] = file.get("scripts", "Download");
  return ret;
}

bool valid_configs(IniDictionary conf) { return config_healthcare(conf) == ""; }

std::string config_healthcare(IniDictionary conf) {
  if (conf.count("name") == 0) {
    return "\"name\" is required!";
  }
  if (conf.count("files") == 0) {
    return "\"files\" is required!";
  }
  if (conf["name"].get_type() != IniType::String) {
    return "\"name\" must be a string!";
  }
  if (conf.count("dependencies") != 0 &&
      conf["dependencies"].get_type() != IniType::List) {
    return "\"dependencies\" must be a list!";
  }
  if (conf["files"].get_type() != IniType::List) {
    return "\"files\" must be a list!";
  }
  if (conf.count("version") != 0 &&
      conf["version"].get_type() != IniType::String) {
    return "\"version\" must be a string!";
  }
  return "";
}

void make_register() {
  if (!std::filesystem::exists(CATCARE_ROOT)) {
    std::filesystem::create_directory(CATCARE_ROOT);
  }
  if (!std::filesystem::exists(CATCARE_ROOT +
                               CATCARE_DIRSLASH CATCARE_REGISTERNAME)) {
    std::ofstream os;
    os.open(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME,
            std::ios::app);
    os << "installed=[]\n";
    os.close();
  }
}

IniList get_register() {
  make_register();
  IniFile reg =
      IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
  return reg.get("installed").to_list();
}

bool installed(std::string name) {
  IniList lst = get_register();
  name = app_username(name);
  for (auto i : lst) {
    if (i.get_type() == IniType::String && name == (std::string)i) {
      return true;
    }
  }
  return false;
}

void add_to_register(std::string name) {
  if (installed(name)) {
    return;
  }
  IniFile reg =
      IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
  IniList l = reg.get("installed").to_list();
  l.push_back("\"" + name + "\"");
  reg.set("installed", l);
  reg.to_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
}

void remove_from_register(std::string name) {
  auto [usr, rname] = get_username(name);
  if (rname != "") {
  }
  if (!installed(name)) {
    return;
  }
  IniFile reg =
      IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
  IniList l = reg.get("installed").to_list();
  for (size_t i = 0; i < l.size(); ++i) {
    if (l[i].get_type() == IniType::String && (std::string)l[i] == name) {
      l.erase(l.begin() + i);
      break;
    }
  }
  reg.set("installed", l);
  reg.to_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
}

bool is_dependency(std::string name) {
  IniList lst = get_dependencylist();
  for (auto i : lst) {
    if (i.get_type() == IniType::String && name == (std::string)i) {
      return true;
    }
  }
  return false;
}

void add_to_dependencylist(std::string name, bool local) {
  IniFile reg = IniFile::from_file(CATCARE_CHECKLISTNAME);
  IniList l = reg.get("dependencies", "Download").to_list();
  for (size_t i = 0; i < l.size(); ++i) {
    if (l[i].get_type() == IniType::String && (std::string)l[i] == name) {
      return;
    }
  }
  IniElement elm;
  if (local)
    elm = "." CATCARE_DIRSLASH + name;
  else
    elm = name;
  l.push_back(elm);
  reg.set("dependencies", l, "Download");
  reg.to_file(CATCARE_CHECKLISTNAME);
}

void remove_from_dependencylist(std::string name) {
  IniFile reg = IniFile::from_file(CATCARE_CHECKLISTNAME);
  IniList l = reg.get("dependencies", "Download").to_list();
  if (!is_dependency(name)) {
    name = "." CATCARE_DIRSLASH + name;
    for (size_t i = 0; i < l.size(); ++i) {
      if (l[i].get_type() == IniType::String && (std::string)l[i] == name) {
        l.erase(l.begin() + i);
        break;
      }
    }
    reg.set("dependencies", l, "Download");
    reg.to_file(CATCARE_CHECKLISTNAME);
    return;
  }

  for (size_t i = 0; i < l.size(); ++i) {
    if (l[i].get_type() == IniType::String && (std::string)l[i] == name) {
      l.erase(l.begin() + i);
      break;
    }
  }
  reg.set("dependencies", l, "Download");
  reg.to_file(CATCARE_CHECKLISTNAME);
}

IniList get_dependencylist() {
  IniFile reg = IniFile::from_file(CATCARE_CHECKLISTNAME);
  return reg.get("dependencies", "Download").to_list();
}

void make_checklist() {
  if (!std::filesystem::exists(CATCARE_CHECKLISTNAME)) {
    make_file(CATCARE_CHECKLISTNAME,
              "[Info]\nname=\"\"\n\n[Download]\ndependencies=[]\nfiles=[]\n");
  }
}

IniList get_filelist() {
  if (std::filesystem::exists(CATCARE_CHECKLISTNAME)) {
    IniFile file = IniFile::from_file(CATCARE_CHECKLISTNAME);
    return file.get("files", "Download");
  }
  return IniList();
}
void set_filelist(IniList list) {
  if (std::filesystem::exists(CATCARE_CHECKLISTNAME)) {
    IniFile file = IniFile::from_file(CATCARE_CHECKLISTNAME);
    file.set("files", list, "Download");
    file.to_file(CATCARE_CHECKLISTNAME);
  }
}

bool blacklisted(std::string repo) {
  IniFile file = IniFile::from_file(CATCARE_CONFIGFILE);
  IniList l = file.get("blacklist");
  for (auto i : l) {
    if (i.get_type() == IniType::String && (std::string)i == repo) return true;
  }
  return false;
}
