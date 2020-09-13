#include <export.h>
#include <stdlib.h>
#include "common.h"

MODULE(dino);
USE(puts);

void main(int argc, char** argv)
{
    module_init();
    INIT(puts);
       
    puts("                         .       .\n");
    puts("                        / `.   .' \\\n");
    puts("                .---.  <    > <    >  .---.\n");
    puts("                |    \\  \\ - ~ ~ - /  /    |\n");
    puts("                 ~-..-~             ~-..-~\n");
    puts("             \\~~~\\.'                    `./~~~/\n");
    puts("              \\__/                        \\__/\n");
    puts("               /                  .-    .  \\\n");
    puts("        _._ _.-    .-~ ~-.       /       }   \\/~~~/\n");
    puts("    _.-'q  }~     /       }     {        ;    \\__/\n");
    puts("   {'__,  /      (       /      {       /      `. ,~~|   .     .\n");
    puts("    `''''='~~-.__(      /_      |      /- _      `..-'   \\\\   //\n");
    puts("                / \\   =/  ~~--~~{    ./|    ~-.     `-..__\\\\_//_.-'\n");
    puts("               {   \\  +\\         \\  =\\ (        ~ - . _ _ _..---~\n");
    puts("               |  | {   }         \\   \\_\\\n");
    puts("              '---.o___,'       .o___,'\n");
}