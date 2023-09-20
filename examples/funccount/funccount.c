#include <elf.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned long int offset;
    char* name;
} Symbol;

int getSymbolOffset(char *filename, Symbol *s, int num);

int main(int argc, char *argv[]) {

    if(argc < 3) {
        printf("usage:");
        printf("\t./funccount <absolute path>/<filename> <user's function name1> ...\n");
        return 0;
    }
    Symbol *symbols = (Symbol*)malloc(sizeof(Symbol) * (argc - 2));
    for(int i = 2; i < argc; i++) {
        symbols[i-2].name = argv[i];
        symbols[i-2].offset = 0;
    }

    getSymbolOffset(argv[1], symbols, argc - 2);
    
    #ifdef DEBUF
    for(int i = 0; i < (argc - 2); i++) {
        printf("%16lx, %s\n", symbols[i].offset, symbols[i].name);
    }
    #endif

    free(symbols);

    return 0;
}

int getSymbolOffset(char *filename, Symbol *s, int num) {

    FILE *fp;
    int numOfSymbol;
    char *shstrtab, *strtab;
    Elf64_Ehdr header;
    Elf64_Shdr shstrtabHeader, symtabHeader, strtabHeader, sheader;
    Elf64_Sym *symbols;

    /* Open file.*/
    fp = fopen(filename, "r");
    if(NULL == fp) {
        fprintf(stderr, "failed to open %s: %s", filename, strerror(errno));
        return -1;
    }

    /* Read header of ELF. */
    fread(&header, sizeof(header), 1, fp);

    /* Read the section header and contents of .shstrtab */
    if(fseek(fp, header.e_shoff + header.e_shstrndx * header.e_shentsize, SEEK_SET) < 0) {
        fprintf(stderr, "failed to set the file position indicator to the section header(.shstrtab): %s", strerror(errno));
        return -1;
    }
    fread(&shstrtabHeader, sizeof(Elf64_Shdr), 1, fp);
    shstrtab = (char*)malloc(sizeof(char) * shstrtabHeader.sh_size);
    if(fseek(fp, shstrtabHeader.sh_offset, SEEK_SET) < 0) {
        fprintf(stderr, "failed to set the file position indicator to the section content(.shstrtab): %s", strerror(errno));
        return -1;
    }
    fread(shstrtab, sizeof(char), shstrtabHeader.sh_size, fp);

    /* Read the section header of .symtab and .strtab */
    for(int i = 0; i < header.e_shnum; i++ ) {
        if(fseek(fp, header.e_shoff + i * header.e_shentsize, SEEK_SET) < 0) {
            fprintf(stderr, "failed to set the file position indicator to the section header(%s): %s", shstrtab + sheader.sh_name, strerror(errno));
            return -1;
        }
        fread(&sheader, sizeof(Elf64_Shdr), 1, fp);

        /* find .strtab*/
        if(0 == strcmp(".strtab", shstrtab + sheader.sh_name)) {
            strtabHeader = sheader;
        }

        /* find .symtab*/
        if(0 == strcmp(".symtab", shstrtab + sheader.sh_name)) {
            symtabHeader = sheader;
        }
    }

    /* Read the content of .strtab */
    strtab = (char*)malloc(sizeof(char) * strtabHeader.sh_size);
    if(fseek(fp, strtabHeader.sh_offset, SEEK_SET) < 0) {
        fprintf(stderr, "failed to set the file position indicator to the section content(.strtab): %s", strerror(errno));
        return -1;
    }
    fread(strtab, sizeof(char), strtabHeader.sh_size, fp);

    /* Read the content of .symtab */
    numOfSymbol = symtabHeader.sh_size/symtabHeader.sh_entsize;
    if(fseek(fp, symtabHeader.sh_offset, SEEK_SET) < 0) {
        fprintf(stderr, "failed to set the file position indicator to the beginning of the section(.symtab): %s", strerror(errno));
        return -1;
    }
    symbols = (Elf64_Sym*)malloc(sizeof(Elf64_Sym) * numOfSymbol);
    fread(symbols, sizeof(Elf64_Sym), numOfSymbol, fp);
    for(int i = 0; i < numOfSymbol; i++) {
        #ifdef DEBUG
        printf("%d: %16lx  %s\n", i, symbols[i].st_value, strtab + symbols[i].st_name);
        #endif
        for(int j = 0; j < num; j++) {
            if(0 == strcmp(s[j].name, strtab + symbols[i].st_name)) {
                s[j].offset = symbols[i].st_value;
            }
        }
    }

    /* Recycle resources. */
    if(0 != fclose(fp)) {
        fprintf(stderr, "failed to close %s: %s", filename, strerror(errno));
        return -1;
    }

    free(shstrtab);
    free(strtab);
    free(symbols);

    return 0;
}