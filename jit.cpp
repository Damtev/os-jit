#include <iostream>
#include <sstream>
#include <vector>
#include <sys/mman.h>
#include <cstring>
#include <string>

/*int sum_by_module(int a, int b) {
    int c = 23;
    return (a + b) % c;
}*/

/*
   0:	55                   	push   %rbp
   1:	48 89 e5             	mov    %rsp,%rbp
   4:	89 7d ec             	mov    %edi,-0x14(%rbp)
   7:	89 75 e8             	mov    %esi,-0x18(%rbp)
   a:	c7 45 fc 17 00 00 00 	movl   $0x17,-0x4(%rbp)
  11:	8b 55 ec             	mov    -0x14(%rbp),%edx
  14:	8b 45 e8             	mov    -0x18(%rbp),%eax
  17:	01 d0                	add    %edx,%eax
  19:	99                   	cltd
  1a:	f7 7d fc             	idivl  -0x4(%rbp)
  1d:	89 d0                	mov    %edx,%eax
  1f:	5d                   	pop    %rbp
  20:	c3                   	retq
*/

typedef int (*func_type)(int, int);

static const std::string help = "Usage:"
                                "\n\t'help' - show this message;"
                                "\n\t'exec' a b - calc a + b by default module 23;"
                                "\n\t'mod' c - change function module to c"
                                "\n\t'exit' - exit the program";

static unsigned char code[] = {
        0x55,                   	                // push     %rbp
        0x48, 0x89, 0xe5,             	            // mov      %rsp,%rbp
        0x89, 0x7d, 0xec,             	            // mov      %edi,-0x14(%rbp)
        0x89, 0x75, 0xe8,             	            // mov      %esi,-0x18(%rbp)
        0xc7, 0x45, 0xfc, 0x17, 0x00, 0x00, 0x00, 	// movl     $0x17,-0x4(%rbp)
        0x8b, 0x55, 0xec,             	            // mov      -0x14(%rbp),%edx
        0x8b, 0x45, 0xe8,             	            // mov      -0x18(%rbp),%eax
        0x01, 0xd0,                	                //add       %edx,%eax
        0x99,                   	                //cltd
        0xf7, 0x7d, 0xfc,             	            //idivl     -0x4(%rbp)
        0x89, 0xd0,                	                //mov       %edx,%eax
        0x5d,                   	                // pop      %rbp
        0xc3                   	                    // retq
};

const size_t code_size = sizeof(code);
const size_t modify_position = 13;

void show_help() {
    std::cout << help << std::endl;
}

void usage_error() {
    std::cout << "Unknown command, read help" << std::endl;
}

void calc(int a, int b, void* fun) {
    if (mprotect(fun, code_size, PROT_EXEC | PROT_READ) == -1) {
        perror("mprotect");
        return;
    }
    auto f = (func_type) fun;
    int result = f(a, b);
    std::cout << "Function result: " << result << std::endl;
}

void patch(int c, void* fun) {
    if (mprotect(fun, code_size, PROT_READ | PROT_WRITE) == -1) {
        perror("mprotect");
        return;
    }

    for (int i = 0; i < 4; i++) {
        code[modify_position + i] = static_cast<unsigned char>(c % 256);
        c /= 256;
    }
    memcpy(static_cast<unsigned char*>(fun) + modify_position, code + modify_position, 4);
}

int main() {

    void* fun = mmap(nullptr, code_size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (fun == MAP_FAILED) {
        std::cout << "Can't mmap to function";
        exit(EXIT_FAILURE);
    }
    std::memcpy(fun, code, code_size);

    show_help();
    std::string command;
    while (std::getline(std::cin, command)) {
        std::vector<std::string> tokens;
        std::istringstream stream(command);
        std::string token;
        while (stream >> token) {
            tokens.push_back(std::move(token));
        }

        if (tokens.empty() || tokens.size() > 4) {
            usage_error();
            continue;
        }
        if (tokens[0] == "help") {
            show_help();
        } else if (tokens[0] == "exit") {
            break;
        } else if (tokens[0] == "exec") {
            if (tokens.size() < 3) {
                usage_error();
                continue;
            }
            int a = std::stoi(tokens[1]);
            int b = std::stoi(tokens[2]);

            calc(a, b, fun);
        } else if (tokens[0] == "mod") {
            if (tokens.size() != 2) {
                usage_error();
                continue;
            }
            int c = std::stoi(tokens[1]);
            patch(c, fun);
        } else {
            usage_error();
            continue;
        }
    }

    if (munmap(fun, code_size) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    return 0;
}