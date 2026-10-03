#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <sys/stat.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#define ONEGIN_FILE "onegin.txt"
#define RESULT_FILE "result.txt"

#define RED   "\033[31m"
#define RESET "\033[0m"

typedef enum {
    ERR_OK                  = 0,
    ERR_MAIN_FOPEN_RESULT   = 1,
    ERR_READFROMFILE_FOPEN  = 2,
    ERR_READFROMFILE_CALLOC = 3,
    ERR_READFROMFILE_READ   = 4,
    ERR_INDEXPASTE_CALLOC   = 5,
    ERR_SIZEFILE_STAT       = 6
} ErrorCode;

enum SwapDecision
{
    NOT_CHANGE = -1,
    EMPTY_STR  =  1,
    CHANGE     =  2
};

typedef struct {
    char* buffer;
    char** index;
    int num_lines;
} FromFile;

FromFile LoadFromFile(const char* file_name);
char* ReadFromFile   (const char* file_name);
int CountStr         (char* buffer);
char** IndexPaste    (char* buffer, int* num_lines);

size_t SizeFile(const char* name);

int BubbleSort   (void* data, int num_elements, size_t size_type, int (*CompareFunc)(void* value1, void* value2));
void ChangeValues(void* value1, void* value2, size_t size_type);

int CompareAlpha  (void* value1, void* value2);
int CompareRhyme  (const void* value1, const void* value2);
int CompareAddress(const void* value1, const void* value2);

void PrintOfResult(FILE * fp, char** index, int num_lines, const char* title);



int main(void)
{
    FromFile text = LoadFromFile(ONEGIN_FILE);
    if (text.buffer == NULL) {
        return 1;
    }

    printf("Number of strings: %d\n", text.num_lines);

    FILE * fp = fopen(RESULT_FILE, "w");
    if (fp == NULL) {
        printf("%s:%d: ", RED "error" RESET " in function %s, code: %d (ERR_MAIN_FOPEN_RESULT)\n",
              __FILE__, __LINE__, __func__, ERR_MAIN_FOPEN_RESULT);
        free(text.buffer);
        free(text.index);
        return ERR_MAIN_FOPEN_RESULT;
    }

    BubbleSort(text.index, text.num_lines, sizeof(char*), CompareAlpha);
    PrintOfResult(fp, text.index, text.num_lines, "1. Alphabetical");

    qsort((void*)text.index, (size_t)text.num_lines, sizeof(char*), CompareRhyme);
    PrintOfResult(fp, text.index, text.num_lines, "2. By rhyme");

    qsort((void*)text.index, (size_t)text.num_lines, sizeof(char*), CompareAddress);
    PrintOfResult(fp, text.index, text.num_lines, "3. Original text");

    printf("Result in file with name <%s>\n", RESULT_FILE);

    fclose(fp);

    free(text.buffer);
    free(text.index);

    return 0;
}


FromFile LoadFromFile(const char* file_name)
{
    FromFile text = {0};
    text.buffer = ReadFromFile(file_name);
    if (text.buffer == NULL) {
        return text;
    }
    text.index = IndexPaste(text.buffer, &text.num_lines);
    if (text.index == NULL) {
        free(text.buffer);
        text.buffer = NULL;
        return text;
    }
    return text;
}

char* ReadFromFile(const char* file_name)
{
    size_t size = SizeFile(file_name);
    size_t size_type = sizeof(char);
    if (size == (size_t)-1) {
        return NULL;
    }

    char* buffer = (char*)calloc(size + 1, size_type);
    if (buffer == NULL) {
        printf("%s:%d: ", RED "error" RESET " in function %s, code: %d (ERR_READFROMFILE_CALLOC)\n",
              __FILE__, __LINE__, __func__, ERR_READFROMFILE_CALLOC);
        free(buffer);
        return NULL;
    }

    FILE * fp = fopen(file_name, "rb");
    if (fp == NULL) {
        printf("%s:%d: ", RED "error" RESET " in function %s, code: %d (ERR_READFROMFILE_FOPEN)\n",
               __FILE__, __LINE__, __func__, ERR_READFROMFILE_FOPEN);
        free(buffer);
        return NULL;
    }

    ssize_t size_useful = read(fileno(fp), buffer, size);
    if (size_useful < 0 || ((size_t)size_useful != size && size != 0)) {
        printf("%s:%d: ", RED "error" RESET " in function %s, code: %d (ERR_READFROMFILE_READ)\n",
              __FILE__, __LINE__, __func__, ERR_READFROMFILE_READ);
        free(buffer);
        fclose(fp);
        return NULL;
    }
    *(buffer + size_useful) = '\0';

    fclose(fp);

    return buffer;
}

int CountStr(char* buffer)
{
    int num_lines = 1;

    for (char* p = buffer; *p; p++) {
        if (*p == '\n' && *(p + 1) != '\0') {
            ++num_lines;
        }
    }

    return num_lines;
}

char** IndexPaste(char* buffer, int* num_lines)
{
    int lines = CountStr(buffer);
    *num_lines = lines;

    char** index = (char**)calloc((size_t)lines, sizeof(char*));
    if (index == NULL) {
        printf("%s:%d: ", RED "error" RESET " in function %s, code: %d (ERR_INDEXPASTE_CALLOC)\n",
              __FILE__, __LINE__, __func__, ERR_INDEXPASTE_CALLOC);
        return NULL;
    }

    index[0] = buffer;
    int i = 0;

    for (int line = 1; *(buffer + i) != '\0' && line < lines; i++)
    {
        if (*(buffer + i) == '\n')
        {
            *(buffer + i) = '\0';
            *(index + line) = buffer + i + 1;
            line++;
        }
    }

    while (*(buffer + i) != '\n' && *(buffer + i) != '\0') i++;
    *(buffer + i) = '\0';

    return index;
}

size_t SizeFile(const char* name)
{
    struct stat m;
    size_t size = 0;

    if (stat(name, &m) != 0) {
        printf("%s:%d: ", RED "error" RESET " in function %s, code: %d (ERR_SIZEFILE_STAT)\n",
              __FILE__, __LINE__, __func__, ERR_SIZEFILE_STAT);
        return (size_t)-1;
    }
    size = m.st_size;

    return size;
}

int BubbleSort(void* data, int num_elements, size_t size_type,
               int (*CompareFunc)(void* value1, void* value2))
{
    int result_comp = 0;
    char* base = (char*)data;

    for (int j = 0; j < num_elements - 1; j++)
    {
        for (int i = 0; i < num_elements - j - 1; i++)
        {
            if ((result_comp = (*CompareFunc)((base + i * size_type), (base + (i + 1) * size_type))) == CHANGE)
            {
                ChangeValues((base + i * size_type), (base + (i + 1) * size_type), size_type);
            }
        }
    }

    return 0;
}

void ChangeValues(void* value1, void* value2, size_t size_type)
{
    unsigned char* p1 = (unsigned char*)value1;
    unsigned char* p2 = (unsigned char*)value2;

    for (int i = 0; i < size_type; i++)
    {
        unsigned char temp = p1[i];
        p1[i] = p2[i];
        p2[i] = temp;
    }
}

int CompareAlpha(void* value1, void* value2)
{
    const char* str1 = *(const char* const*)value1;
    const char* str2 = *(const char* const*)value2;
    int i = 0, j = 0;

    while (*(str1 + i) != '\0' && *(str2 + j) != '\0')
    {
        while (*(str1 + i) != '\0' && !isalpha(*(str1 + i))) i++;
        while (*(str2 + j) != '\0' && !isalpha(*(str2 + j))) j++;

        char let1 = tolower(*(str1 + i));
        char let2 = tolower(*(str2 + j));

        if (let1 < let2) return NOT_CHANGE;
        if (let1 > let2) return CHANGE;

        i++;
        j++;
    }

    return EMPTY_STR;
}

int CompareRhyme(const void* value1, const void* value2)
{
    const char* str1 = *(const char* const*)value1;
    const char* str2 = *(const char* const*)value2;
    int i = 0, j = 0;

    while (*(str1 + i) != '\0') i++;
    while (*(str2 + j) != '\0') j++;

    while (i >= 0 && j >= 0)
    {
        while (i >= 0 && !isalpha(*(str1 + i))) i--;
        while (j >= 0 && !isalpha(*(str2 + j))) j--;

        if (i < 0 || j < 0) break;

        char let1 = tolower(*(str1 + i));
        char let2 = tolower(*(str2 + j));

        if (let1 != let2) return let1 - let2;
        if (let1 > let2) return CHANGE;
        if (let1 < let2) return NOT_CHANGE;

        i--;
        j--;
    }

    return EMPTY_STR;
}

int CompareAddress(const void* value1, const void* value2)
{
    const char* addr1 = *(const char* const*)value1;
    const char* addr2 = *(const char* const*)value2;

    if (addr1 < addr2) return NOT_CHANGE;
    if (addr1 > addr2) return CHANGE;
    return 0;
}

void PrintOfResult(FILE* fp, char** index, int num_lines, const char* title)
{
    fprintf(fp, "<%s>\n", title);
    for (int i = 0; i < num_lines; i++)
        fprintf(fp, "%s\n", index[i]);
    fprintf(fp, "\n\n");
}
