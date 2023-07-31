# WebCrawler
a Web Crawler project as a multi-threaded application under Linux OS (Ubuntu) by using C++ and bash-terminal

Memory Organization:
I used “std::unique_ptr<std::string>” to make memory management safer, preventing 
memory leaks and invalid memory accesses.
I implemented checks on memory boundaries to prevent buffer overflows. [if (len > 
(std::numeric_limits<size_t>::max() - str->size()))]
I minimized the lock and unlock times of mutexes to improve performance.
Optimizable Coding:
I used the full names of standard library elements instead of the “using namespace std;”
statement. This way, I prevented potential name clashes and made my code more readable.
I made the shared variable “stopCrawling” safer for multiple threads by defining it as 
“std::atomic<bool>” type. This ensures that the variable is accessed in a thread-safe manner.
GDB:
used the commands ‘run,’ ‘break,’ ‘delete,’ and ‘print’
