#include "network.hpp"
#include "configs.hpp"
#include "options.hpp"

#include "carescript.hpp"

#ifdef __linux__
#include <curl/curl.h>
#include <unistd.h>
#include <pwd.h>

bool download_page(std::string url, std::string file) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    const char* aurl = url.c_str();
    const char* outfilename = file.c_str();
    curl = curl_easy_init();                                                                                                                                                                                                                                                           
    if (curl) {   
        fp = fopen(outfilename,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, aurl);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
        if(res != 0) {
            std::filesystem::remove_all(file);
            return false;
        }
        return true;
    }   
    return false;
}

std::string get_username() {
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        return std::string(pw->pw_name);
    }
    return "";
}

#elif defined(_WIN32)
#include <windows.h>
//#include <Lcmd.h>
#include <codecvt>

bool download_page(std::string url, std::string file) {
    const wchar_t* srcURL = std::wstring(url.begin(),url.end()).c_str();
    const wchar_t* destFile = std::wstring(file.begin(),file.end()).c_str();

    //if (S_OK == URLDownloadToFile(NULL, srcURL, destFile, 0, NULL)) {
        return 0;
    //}
   // return 1;
}

std::string get_username() {
return "h";
}

#else
std::string get_username() {}
bool download_page(std::string url, std::string file) {}
#endif


void download_dependencies(IniList list) {
    for(auto i : list) {
        if(i.get_type() == IniType::String && !installed((std::string)i)) {
            std::string is = i;
            if(is.size() > 2 && is[0] == '.' && is[1] == CATCARE_DIRSLASHc) {
                print_message("COPYING","Copying local dependency: \"" + is + "\"");
                std::string error = get_local(
                    last_name(is),
                    std::filesystem::path(is).parent_path().string());
                if(error != "")
                    print_message("ERROR","Error copying local repo: \"" + is + "\"\n-> " + error);
            }
            else {
                print_message("DOWNLOAD","Downloading dependency: \"" + (std::string)i + "\"");
                std::string error = download_repo((std::string)i);
                if(error != "")
                    print_message("ERROR","Error downloading repo: \"" + (std::string)i + "\"\n-> " + error);
            }
        }
    }
}

#define CLEAR_ON_ERR() if(option_or("clear_on_error","true") == "true") {std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);}
#define IFERR(err) if(err != "") { CLEAR_ON_ERR(); return err; }

bool download_scripts(IniList scripts,std::string install, std::string name, std::string usr, std::map<std::string,ScriptLabel> labels = {}) {
    for(auto i : scripts) {
        if(i.get_type() == IniType::String) {
            print_message("DOWNLOAD","Downloading script: " + (std::string)i);
            if(!labels.empty()) {
                std::string err = run_script("download_script",labels,"",{
                    usr,
                    name,
                    (std::string)i,
                    CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + (std::string)i
                });
                if(err != "") {
                    print_message("ERROR","Download Script failed: " + err);
                }
            }
            else if(!download_page(CATCARE_REPOFILE(install,(std::string)i),CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + (std::string)i)) {
                print_message("ERROR","Failed to download script!");
                continue;
            }

            std::string source;
            std::ifstream ifile(CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + (std::string)i);
            while(ifile.good()) source += ifile.get();
            if(source != "") source.pop_back();

            if(option_or("show_script_src","false") == "true") {
                KittenLexer line_lexer = KittenLexer()
                    .add_extract('\n');
                auto lexed = line_lexer.lex(source);
                std::cout << "> Q to exit, S to stop download, enter to continue\n";
                std::cout << "================> Script " << (std::string)i << " | lines: " << lexed.back().line << "\n";
                std::string inp;
                int l = 0;
                do {
                    while(lexed[l].src != "\n" && l != lexed.size()-1) {
                        std::cout << lexed[l].line << ": " << lexed[l].src;
                        ++l;
                    }
                    ++l;
                    std::getline(std::cin,inp);
                } while(inp != "q" && inp != "Q" && inp != "S" && inp != "S" && l == lexed.size());
                if(inp == "s" || inp == "S") {
                    print_message("INFO","\nExiting download");
                    CLEAR_ON_ERR();
                    return true;
                }
                std::cout << "================> Enter to continue...";
                std::getline(std::cin,inp);
            }

            print_message("INFO","Entering CCScript: \"" + (std::string)i + "\"");
            std::string err = run_script(source);
            if(err != "") {
                print_message("ERROR","Script failed: " + err);
            }
        }
    }
    return false;
}

std::string download_repo(std::string install) {
    make_register();
    make_checklist();
    install = to_lowercase(install);
    if(blacklisted(install)) {
        return "This repo is blacklisted!";
    }
    auto [usr,name] = get_username(install);
    if(usr == "") usr = CATCARE_USER;
    install = app_username(install);
    if(std::filesystem::exists(CATCARE_ROOT + CATCARE_DIRSLASH + name)) {
        std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
    }
    std::filesystem::create_directories(CATCARE_ROOT + CATCARE_DIRSLASH + name);
    std::filesystem::create_symlink(".." CATCARE_DIRSLASH ".." CATCARE_DIRSLASH + CATCARE_ROOT, CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + CATCARE_ROOT);
    
    if(std::filesystem::exists(CATCARE_DOWNLOADSCRIPT)) {
        std::ifstream ifile(CATCARE_DOWNLOADSCRIPT);
        std::string s;
        while(ifile.good()) s += ifile.get();
        if(!s.empty()) s.pop_back();
        if(s.empty()) goto DEFAULT_DOWNLOAD;

        auto labels = into_labels(s);

        std::string err = run_script("init",labels,"",{
            usr, // username
            name, // project-name
        });
        IFERR(err);
        
        // err = run_script("download_checklist",labels,"",{
        //     usr, // user
        //     name, // project
        //     CATCARE_CHECKLISTNAME, // filename
        //     CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + CATCARE_CHECKLISTNAME, // destination
        // });
        IFERR(err);

        IniDictionary conf = extract_configs(name);
        if(!valid_configs(conf)) {
            CLEAR_ON_ERR()
            return config_healthcare(conf);
        }
        IniList files = conf["files"].to_list();

        for(auto i : files) {
            if(i.get_type() != IniType::String) {
                continue;
            }
            std::string file = (std::string)i;
            if(file.size() == 0) {
                continue;
            }

            if(file[0] == '$') {
                file.erase(file.begin());
                if(file.empty()) {
                    continue;
                }
                print_message("DOWNLOAD","Adding dictionary: " + file);
                std::filesystem::create_directory(CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + file);
                continue;
            }
            else if(file[0] == '#') {
                continue;
            }
            else if(file[0] == '!') {
                file.erase(file.begin());
                if(file.empty()) {
                    continue;
                }
                std::string ufile = last_name(file);
                print_message("DOWNLOAD","Downloading file: " + ufile);
                run_script("download_file",labels,"",{
                    usr,
                    name,
                    file,
                    CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + ufile,
                });
                IFERR(err);
            }
            else {
                print_message("DOWNLOAD","Downloading file: " + file);
                run_script("download_file",labels,"",{
                    usr,
                    name,
                    file,
                    CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + file,
                });
                IFERR(err);
            }
        }

        if(conf.count("scripts") != 0 && option_or("no_scripts","false") == "false") {
            IniList scripts = conf["scripts"].to_list();

            if(download_scripts(scripts,install,name,usr,labels)) {
                return "";
            }
        }

        err = run_script("finish",labels,"",{
            usr, // username
            name, // project-name
        });
        IFERR(err);
        

        if(!installed(install)) {
            add_to_register(install);
        }

        download_dependencies(conf["dependencies"].to_list());

        return "";
    }
DEFAULT_DOWNLOAD:
    if(!download_page(CATCARE_REPOFILE(install,CATCARE_CHECKLISTNAME),CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME)) {
        CLEAR_ON_ERR()
        return "Could not download checklist!";
    
    }
    IniDictionary conf = extract_configs(name);
    if(!valid_configs(conf)) {
        CLEAR_ON_ERR()
        return config_healthcare(conf);
    }
    IniList files = conf["files"].to_list();

    for(auto i : files) {
        if(i.get_type() != IniType::String) {
            continue;
        }
        std::string file = (std::string)i;
        if(file.size() == 0) {
            continue;
        }

        if(file[0] == '$') {
            file.erase(file.begin());
            if(file.empty()) {
                continue;
            }
            print_message("DOWNLOAD","Adding dictionary: " + file);
            std::filesystem::create_directory(CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + file);
            continue;
        }
        else if(file[0] == '#') {
            continue;
        }
        else if(file[0] == '!') {
            file.erase(file.begin());
            if(file.empty()) {
                continue;
            }
            std::string ufile = last_name(file);
            print_message("DOWNLOAD","Downloading file: " + ufile);
            if(!download_page(CATCARE_REPOFILE(install,file),CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + ufile)) {
                CLEAR_ON_ERR()
                return "Error downloading file: " + ufile;
            }
        }
        else {
            print_message("DOWNLOAD","Downloading file: " + file);
            if(!download_page(CATCARE_REPOFILE(install,file),CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + file)) {
                CLEAR_ON_ERR()
                return "Error downloading file: " + file;
            }
        }
    }

    if(conf.count("scripts") != 0 && option_or("no_scripts","false") == "false") {
        IniList scripts = conf["scripts"].to_list();

        if(download_scripts(scripts,install,name,usr)) {
            return "";
        }
    }

    if(!installed(install)) {
        add_to_register(install);
    }

    download_dependencies(conf["dependencies"].to_list());

    return "";
}

std::string get_local(std::string name, std::string path) {
    make_register();
    make_checklist();
    name = to_lowercase(name);
    if(std::filesystem::exists(CATCARE_ROOT + CATCARE_DIRSLASH + name)) {
        std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
    }
    std::filesystem::create_directories(CATCARE_ROOT + CATCARE_DIRSLASH + name);
    
    try {
        std::filesystem::copy(path + CATCARE_DIRSLASH + CATCARE_CHECKLISTNAME,CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    }
    catch(...) {
        std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
        return "Could not copy checklist!";
    }
    IniDictionary conf = extract_configs(name);
    if(!valid_configs(conf)) {
        std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
        return config_healthcare(conf);
    }
    IniList files = conf["files"].to_list();

    for(auto i : files) {
        if(i.get_type() != IniType::String) {
            continue;
        }
        std::string file = (std::string)i;
        if(file.size() == 0) {
            continue;
        }

        if(file[0] == '$') {
            file.erase(file.begin());
            if(file.empty()) {
                continue;
            }
            print_message("COPYING","Adding dictionary: " + file);
            std::filesystem::create_directory(CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + file);
            continue;
        }
        else if(file[0] == '#') {
            continue;
        }
        else if(file[0] == '!') {
            file.erase(file.begin());
            if(file.empty()) {
                continue;
            }
            std::string ufile = last_name(file);
            print_message("COPYING","Copying file: " + file);
            try {
                std::filesystem::copy(path + CATCARE_DIRSLASH + file,CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + ufile);
            }
            catch(...) {
                std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
                return "Error downloading file: " + file;
            }
        }
        else {
            print_message("COPYING","Downloading file: " + file);
            try {
                std::filesystem::copy(path + CATCARE_DIRSLASH + file,CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + file);
            }
            catch(...) {
                std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
                return "Error downloading file: " + file;
            }
        }
    }

    if(!installed(name)) {
        add_to_register(name);
    }
    for(auto i : conf["dependencies"].to_list()) {
        if(i.get_type() == IniType::String && !installed((std::string)i)) {
            print_message("COPYING","Downloading dependency: \"" + (std::string)i + "\"");
            std::string error;
            if(option_or("local_over_url","false") == "true") {
                if(is_redirected((std::string)i)) {
                    error = get_local((std::string)i,local_redirect[(std::string)i]);
                }
                else {
                    error = get_local(last_name((std::string)i),std::filesystem::path((std::string)i).parent_path().string());
                }
                if(error != "") {
                    error = download_repo((std::string)i);
                }
            }
            else {
                error = download_repo((std::string)i);
                if(error != "") {
                    if(is_redirected((std::string)i)) {
                        error = get_local((std::string)i,local_redirect[(std::string)i]);
                    }
                    else {
                        error = get_local(last_name((std::string)i),std::filesystem::path((std::string)i).parent_path().string());
                    }
                }
            }

            if(error != "")
                print_message("ERROR","Error copying project: \"" + (std::string)i + "\"\n-> " + error);
        }
    }

    // download_dependencies(conf["dependencies"].to_list());

    return "";
}

#define REMOVE_TMP() std::filesystem::remove_all(CATCARE_TMPDIR)
#define RETURN_TUP(a,b) return std::make_tuple<std::string,std::string>(a,b)

std::tuple<std::string,std::string> needs_update(std::string name) {
    name = app_username(name);
    if(!installed(name)) RETURN_TUP("","");
    auto [usr,proj] = get_username(name);

    std::filesystem::create_directories(CATCARE_TMPDIR);
    download_page(CATCARE_REPOFILE(name,CATCARE_CHECKLISTNAME),CATCARE_TMPDIR CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    IniFile r = IniFile::from_file(CATCARE_TMPDIR CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    if(!r || !r.has("version","Info")) {
        REMOVE_TMP(); RETURN_TUP("","");
    }

    auto newest_version = r.get("version","Info");
    if(newest_version.get_type() != IniType::String) { REMOVE_TMP(); RETURN_TUP("",""); }

    r = IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH + proj + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    if(!r || !r.has("version","Info")) { REMOVE_TMP(); RETURN_TUP(newest_version.to_string(),"???"); }

    auto current_version = r.get("version","Info");
    if(current_version.get_type() != IniType::String) {
        REMOVE_TMP(); RETURN_TUP(newest_version.to_string(),"???");
    }
    
    if(newest_version.to_string() != current_version.to_string()) {
        REMOVE_TMP(); RETURN_TUP(newest_version.to_string(),current_version.to_string());
    }

    REMOVE_TMP();
    RETURN_TUP("","");
}

#undef REMOVE_TMP
#undef RETURN_TUP