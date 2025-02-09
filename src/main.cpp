/*
                                                                          
                                                                          
MMMMMMMM               MMMMMMMM                    iiii                   
M:::::::M             M:::::::M                   i::::i                  
M::::::::M           M::::::::M                    iiii                   
M:::::::::M         M:::::::::M                                           
M::::::::::M       M::::::::::M  aaaaaaaaaaaaa   iiiiiiinnnn  nnnnnnnn    
M:::::::::::M     M:::::::::::M  a::::::::::::a  i:::::in:::nn::::::::nn  
M:::::::M::::M   M::::M:::::::M  aaaaaaaaa:::::a  i::::in::::::::::::::nn 
M::::::M M::::M M::::M M::::::M           a::::a  i::::inn:::::::::::::::n
M::::::M  M::::M::::M  M::::::M    aaaaaaa:::::a  i::::i  n:::::nnnn:::::n
M::::::M   M:::::::M   M::::::M  aa::::::::::::a  i::::i  n::::n    n::::n
M::::::M    M:::::M    M::::::M a::::aaaa::::::a  i::::i  n::::n    n::::n
M::::::M     MMMMM     M::::::Ma::::a    a:::::a  i::::i  n::::n    n::::n
M::::::M               M::::::Ma::::a    a:::::a i::::::i n::::n    n::::n
M::::::M               M::::::Ma:::::aaaa::::::a i::::::i n::::n    n::::n
M::::::M               M::::::M a::::::::::aa:::ai::::::i n::::n    n::::n
MMMMMMMM               MMMMMMMM  aaaaaaaaaa  aaaaiiiiiiii nnnnnn    nnnnnn
                                                                          
                                                                          
                                                                          
                                                                          
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 */
/** C++ std library */
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "spinners.hpp"

//#include <windows.h>
//#include <mmsystem.h>


/** C std library and Unix headers (mainly used in function doTask()) */
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include "unistd.h"
#include <process.h> 
#elif __linux__
#include <unistd.h>
#endif

//#include <sys/wait.h>
#include <sys/types.h>

#include "argparse.hpp"
#include "core/framework.hpp"

//#include "core/parser.h"
#include "script/carescript.hpp"


//using namespace parser;
using namespace framework;
using namespace std;

using std::endl;
using std::cerr;
using std::cout;
using std::string;
using std::vector; /** extendable array */
using std::getline; /** reads a line from a file and puts it in a string */
using std::ostream;
using std::ifstream; /** class representing a file to read */
using std::unordered_map; /** C++'s hash table */
using std::unordered_set; /** C++'s hash set */

void read_pieScript() {
  std::clock_t c_start = std::clock();  // Track Time Taken
  // Start read file
  std::string r;
  std::ifstream ifile("build.pie");

  while (ifile.good()) r += ifile.get();
  if (!r.empty()) r.pop_back();

  auto labels = into_labels(r);
  for (auto i : labels) {
    std::cout << i.first << " args: " << i.second.arglist.size()
              << " lines: " << i.second.lines.size() << "\n";
    std::cout << run_script(i.first, labels) << "\n";
  }
  std::clock_t c_end = std::clock();

  double time_elapsed_ms = 1000.0 * (c_end - c_start) / CLOCKS_PER_SEC; //Calulate how much time taken
  std::cout << "CPU time used: " << time_elapsed_ms << " ms\n";
}





/** Target is a DAG node; it has a vertex ID (name), some edges (adjacent) and
    data (tasks). */
class Target
{
public:
    Target(const string& name): name(name) { };
    string name;
    vector<string> adjacent;
    vector<string> tasks;

    friend ostream& operator<<(ostream& out, const Target& t);
};

/** to type less, create shorter aliases for these templated class names */
typedef unordered_set<string> StringSet;
typedef unordered_map<string, Target> TargetMap;

/** overloaded output operator prints a vector of strings. Note that
    std::ostream is the TYPE of the cout object! In fact, defining this
    operator overload allows us to say: cout << someVector

    Why does this function also RETURN an ostream?? See the comments above
    main() */
ostream& operator<<(ostream& out, const vector<string>& v) {
  out << "[";
  for (auto it = v.begin(); it != v.end(); ++it) {
    out << *it;
    if (it + 1 != v.end()) out << ", ";
  }
  out << "]";
  return out;
}

/** overloaded output operator prints a Target (using the overload above, since
    a Target contains 2 vectors) */
ostream& operator<<(ostream& out, const Target& t) {
  out << t.name << ": " << t.tasks << ", " << t.adjacent;
  return out;
}

/** return a COPY of the string with spaces trimmed from the left and right */
string trimmed(const string& str, const string& rem=" ")
{
    /** C++ containers each have a <container>::size_type, which is a nested
        numeric type big enough to describe the container's size (think of
        string::size_type as an int)

        This trim function 1) finds the FIRST occurence (integer position in
        the string) of a char NOT in rem (string containing the char's we'd
        like to trim off) then 2) finds the LAST occurence of a chat NOT in rem
        3) the substring [FIRST, LAST] is what we return

        @EXTRA:
        https://en.cppreference.com/w/cpp/string/basic_string/find_first_not_of
        https://en.cppreference.com/w/cpp/string/basic_string/find_first_not_of
        for the string function docs */
    string::size_type begin = str.find_first_not_of(rem);

    /** No characters besides rem exists, return empty
        Note string::npos is a constant which indicates "couldn't be found" */
    if (begin == string::npos)
        return "";

    /** take the substring from FIRST occurence to LAST occurence of characters
        which are NOT in rem, as discussed above */
    return str.substr(begin, str.find_last_not_of(rem) - begin + 1);
}

/** read each line of a file and put its trimmed lines into a vector */
bool readFile(const string& path, vector<string>& lines)
{
    ifstream file(path);
    if (!file)
        return false;

    string line;
    while (getline(file, line))
        lines.push_back(trimmed(line));

    /** return whether read succeded, let destructor close the file (RAII)

        @EXTRA: There are 2 ways to close a file; either explicitly by calling
        file.close() or implcitly via the destructor. Usually in C++ just let
        the destructor do it. See these links for some interesting discussion.

        (Specifically, Martin York's answer) https://stackoverflow.com/questions/748014/do-i-need-to-manually-close-an-ifstream
        (Again, Martin York's answer) https://codereview.stackexchange.com/questions/540/open-write-and-close-a-file
        https://stackoverflow.com/questions/16608783/handling-c-read-only-file-close-errors
        */
    return !file.bad();
}

/** print an error and line number */
void lineError(unsigned int line, const string& msg) {
  cerr << "Error: " << msg << " [line " << line << "]\n";
}

/** print an error for a target */
void targetError(const string& target) {
  cerr << "Error: processing target [" << target << "]\n";
}

/** print an error for a task */
void taskError(const string& task) {
  cerr << "Error: processing task [" << task << "]\n";
}

/** "adjacent" refers to tokens after the colon in the Cakefile, these are
    dependencies... i.e. other targets the current one depends on. All we're
    doing here is 1) taking the line after the colon 2) splitting that line
    into tokens based on spaces (C++ has no std::split() so we make our own)
    and inserting each token (i.e. the string name of a target that the current
    target depends on) and 3) insert each token into the target's adjacent
    vector

    Therefore, the adjacent vector contains the NAMES of the targets that the
    current one depends on, i.e. the NEIGHBOURS in the DEPENDENCY DAG

    e.g.
    myTarget: dep1 dep2
        task1
        task2

    Here, we would add tokens dep1 and dep2 to Target::adjacent */
void parseAdjacent(string adj, Target& tgt) {
  string::size_type pos;

  /** while there are still spaces */
  while ((pos = adj.find(" ")) != string::npos) {
    /** take the string from index 0 to the first space */
    string token = adj.substr(0, pos);

    /** erase it AND the space */
    adj.erase(0, pos + 1);

    /** if the token extracted is non-empty, add to the adjacency vector */
    if (token.size()) tgt.adjacent.push_back(token);
  }

  /** don't forget the last token! */
  if (adj.size()) tgt.adjacent.push_back(adj);
}

/** a task is a command listed with a tab (see Cakefile)... all we do here is
    1) take non-empty 2) strings which start with a tab and 3) insert them
    into the target's tasks vector
    
    returns whether the current target is fully parsed or not

    e.g.
    myTarget: dep1 dep2
        task1

        task2
        task3

    Here, we parse the whole indented block (skipping empty/space-only lines),
    adding task1, task2 and task3 to Target::tasks returning false at task3 */
bool parseTask(const string& t, vector<string>& tasks)
{
    /** skip empty lines or lines with only spaces */
    if (t.find_first_not_of(' ') == string::npos)
        return true;

    /** if line doesn't start with tab, it's not part of the current target */
    if (t.at(0) != '\t')
        return false;

    /** read the task line, skipping the leading tab character */
    tasks.push_back(t.substr(1));
    return true;
}

/** build a map of Targets by parsing the Cakefile lines, a line that 1) is not
    part of a target's indented tasks block 2) is non-empty and 3) does not
    have a colon is an ERROR! If 1) == true && 2) == true && we FOUND a colon,
    interpret that line as a new target and place it in the map

    e.g.
    Once we reach a line like this:
    myTarget: dep1 dep2
        ...tasks...

    Then we call parseAdjacent on the first line parseTask in a loop to read
    the indented ...tasks... block */
bool parseTargets(TargetMap& nodes, vector<string>& lines)
{
    unsigned int lineNo = 1;
    auto it = lines.begin();

    while (it != lines.end()) {
        if (it->find_first_not_of(' ') == string::npos) {
            ++it; ++lineNo;
            continue;
        }

        auto pos = it->find(":");
        if ((pos == string::npos) || !pos) {
            lineError(lineNo, "no target");
            return false;
        }

        string name = it->substr(0, pos);

        /** emplace() is a method which adds to the hash table
            (TargetMap == unordered_map == hash table)

            It is a VARIADIC function where the first argument is the KEY and
            all subsequent arguments are forwarded to the constructor of the
            Target which will be created in the hash table

            The python equivalent:

            nodes = dict()
            nodes[name] = Target(name) # notice Target::Target() accepts name */
        nodes.emplace(name, name);
        parseAdjacent(it->substr(pos + 1), nodes.at(name));

        /** 1) pre-increment the iterator 2) compare it with lines.end()
            3) if we're not at the end yet, we should increment the lineNo
            (used for error reporting)

            technically ++lineNo could be in the while loop body, I'd like to
            emphasize the "it" and "lineNo" are ALWAYS incremented together,
            (see the if statement above, they're also incremented together)
            hence putting it in the while loop condition*/
        while (++it != lines.end() && ++lineNo) {
            if (!parseTask(*it, nodes.at(name).tasks))
                break;
        }
    }

    return true;
}

/** DFS (depth first search), push the current target into the vector "order"
    after parsing all its adjacent nodes (dependencies!!) first... this makes
    sense since we can't finish the current target until those dependencies are
    done first */
void topologicalSort
(
    TargetMap& nodes,
    const string& id,
    StringSet& visited,
    vector<string>& order
)
{
    if (visited.find(id) != visited.end())
        return;

    visited.emplace(id);
    for (const string& adj : nodes.at(id).adjacent)
        topologicalSort(nodes, adj, visited, order);
    order.push_back(id);
}

/** one way to do topological sort is doing a DFS on each connected component
    in the DAG, that's why we iterate through the TargetMap */
void sortTargets(TargetMap& nodes, vector<string>& order)
{
    StringSet visited;
    for (auto& p : nodes)
        topologicalSort(nodes, p.first, visited, order);
}

/** the stuff in this function uses Unix "system calls" to execute tasks...
    specifically we first "fork" a child process and let the process "exec"
    the command string using the BASH shell, i.e. bash -c "some command" */
bool doTask(string task) {
  cout << "@" << task << endl;

  system(task.c_str());

  /** like Make, a command is successful if it exited with code 0 */
  return true;  // WIFEXITED(status) && WEXITSTATUS(status) == 0
}

/** loop through all tasks in the target */
bool processTarget(Target& tgt)
{
    for (const string& task : tgt.tasks) {
        if (!doTask(task)) {
            //taskError(task);
            return true;
        }
    }

    return true;
}

/** loop through all targets */
bool processTargets(TargetMap& nodes, vector<string>& order)
{
    for (const string& name : order) {
        if (!processTarget(nodes.at(name))) {
            targetError(name);
            return false;
        }
    }

    return true;
}

/*******************************************************************************
 * @SUMMARY:
 *
 * 1. read the Piefile
 * 2. build a map of Target objects from the lines
 * 3. do a topological sort on the map (the map is a DAG)
 * 4. iterate through the "order" vector, which represents the order in which
 *    tasks should be done to satisfy the dependencies you defined
 *
 *******************************************************************************
 * @TIPS:
 *
 * (1)
 * Notice objects are often passed by reference when changes need to be visible
 * to the caller, e.g. readFile() accepts the vector by reference!
 *
 * (2)
 * Keyword "auto" is like "var" or "let" in other languages, it's used when we
 * don't want to type a long type name, i.e. the compiler "infers" the type.
 *
 * (3)
 * TargetMap and StringSet are aliases for templated classes
 * unordered_map<string, Target> and unordered_set<string> for convenience.
 *
 * (4)
 * Function "perror()" is from the C std library, it prints a message for the
 * current value of errno. What is errno you may ask?? Good question!! It is a
 * global numeric variable mandated by both the POSIX operating system standard
 * as well as the C standard, which is used to report errors occuring in system
 * calls. E.g. after trying to opening a file with a bad path, errno may be set
 * to EBADF (or something, I don't remember)... The point is perror() uses the
 * value of errno to print an associated string message. A bunch of error codes
 * are defined as macros in <errno.h>.
 *
 * [TLDR] If file read fails, we call perror(), which will print a relevant
 * message since I/O errors are reported via setting errno by the underlying
 * system calls used by ifstream. If this doesn't make sense to you, don't
 * worry!! The fantastic world of system programming in C/C++ can be learned in
 * good time...
 *
 * (5)
 * The convention if (!doSomething())... is inherited from C and also commonly
 * used in C++. Basically doSomething() is a function returning a bool true for
 * success, false for failure. If it fails (! operator) then we should probably
 * log an error message. Basically the act of DOING and the CHECKING of its
 * result are combined into a condition... Yes this is strange but the spirit
 * of C (and to an extent C++) is generally to be more terse if possible.
 *
 * @FunFact:
 * In a way, Java's verbose naming styles are a REACTION to some of the
 * difficulties programming in C, where a function may be called something
 * obscure like htons() (host to network short is a function in the Unix
 * sockets API <arpa/inet.h>)
 *
 * (6)
 * the "friend ostream& operator<<..." declaration in class Target is kind of
 * like __str__ from Python, except here it is a FUNCTION which prints a
 * Target instance. The "friend" keyword means the function can access PRIVATE
 * members of the Target and is a convention used in C++ (although yes, in
 * this case our class has no private data... but if we added any it could be
 * printed by the friend function)
 *
 * (7)
 * How come the operator<< functions RETURN an ostream& ???? Notice that those
 * functions accept an ostream& out, well that same input parameter is
 * returned!
 *
 * cout << "a" << "b";
 * is equivalent to
 * ( cout << "a" ) << "b"
 *
 * i.e. the NORMAL << operator returns the output stream! This is what makes it
 * possible to CHAIN multiple << together. Hence when overriding <<, we must
 * return an ostream& to be able to chain multiple prints together when using
 * our OVERLOADED operators.
 *
 * (8)
 * std::string has a method c_str() which returns a C-style string, i.e. char *
 * This is useful because many of the operating system API's (e.g. execvp) are
 * desgined to work with C-strings.
 ******************************************************************************/
int make()
{
    vector<string> lines;
    if (!readFile("Piefile", lines)) {
        perror("readFile"); //Can not fine the Piefile
        return 1;
    }

    /** (1) uncomment this block, it'll print the lines read from Piefile */
    // for (string& l : lines)
    //    cout << l << "\n";
    // return 0;

    TargetMap nodes;
    if (!parseTargets(nodes, lines))
        return 1;

    /** (2) prints the Target objects parsed from the vector of file lines */
    // for (auto& p : nodes)
    //     cout << "(" << p.first << ") " << p.second << "\n";
    // return 0;

    vector<string> order;
    sortTargets(nodes, order);

    /** prints the order targets will be processed (topological) */
    cout << "[...Target Order...]\n";
    for (auto& name : order)
        cout << name << "\n";
    // return 0;

    cout << "[...Processing...]\n";
    return processTargets(nodes, order) ? 0 : 1;
}


// argparse::ArgumentParser program("test");

// program.add_argument("--build")
//     .help("build root dir")
//     .default_value(false)
//     .implicit_value(true);

// try {
//   program.parse_args(argc, argv);
// } catch (const std::runtime_error& err) {
//   std::cerr << err.what() << std::endl;
//   std::cerr << program;
//   std::exit(1);
// }

// if (program["--build"] == true) {
//   //   std::cout << "Building" << std::endl;
//   std::clock_t c_start = std::clock();  // Track Time Taken
//   // Start read file
//   std::string r;
//   std::ifstream ifile("build.pie");
//   while (ifile.good()) r += ifile.get();
//   if (!r.empty()) r.pop_back();
//   auto labels = into_labels(r);
//   for (auto i : labels) {
//     std::cout << i.first << " args: " << i.second.arglist.size()
//               << " lines: " << i.second.lines.size() << "\n";
//     std::cout << run_script(i.first, labels) << "\n";
//   }
//   std::clock_t c_end = std::clock();

//   double time_elapsed_ms = 1000.0 * (c_end - c_start) / CLOCKS_PER_SEC;
//   std::cout << "CPU time used: " << time_elapsed_ms << " ms\n";
// }

// parser_class parser_class;
// parser_class.parser_file("test.pie");





int main(int argc, char* argv[]) {
    //Beep(1000,100);
//   jms::Spinner s("Doing something cool", jms::classic);
//   s = jms::Spinner("Now Starting the next task", jms::classic);
//   s.start();

//   this_thread::sleep_for(2s);
//   s.setAnimation(jms::dots);
//   this_thread::sleep_for(2s);

//   s.finish(jms::FinishedState::SUCCESS, "Failed to finish that task");
  // Intialization
  // Initialize();

  argparse::ArgumentParser program("pie");

  program.add_argument("--build")
      .help("build root dir")
      .default_value(false)
      .implicit_value(true);

  program.add_argument("--make")
      .help("Run Piefile in the root dir")
      .default_value(false)
      .implicit_value(true);

  program.add_argument("--download")
      .default_value(std::string("none"))
      .help("Downloads a repo (repository) in the root dir")
      .default_value(false)
      .implicit_value(true);

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  if (program["--build"] == true) {
    read_pieScript();
  }
  if (program["--make"] == true) {
    make();
  }
  if (program["--download"] == true) {
    auto input = program.get<string>("--download");
        //     std::string error;
        // std::string repo = input;
        // repo = to_lowercase(repo);
        // error = download_repo(repo);

  }

  return 0;
}