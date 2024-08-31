#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct word{
  char * contents;
  int numChars;
  int frequency;
  int orderAppeared;
  struct word * nextWord;
  struct word * prevWord;
} WORD;



void printUsage() {
    printf("USAGE:\n\t./MADCounter -f <input file> -o <output file> -c -w -l -Lw -Ll\n\t\tOR\n\t./MADCounter -B <batch file>\n");
}

typedef struct charInfo{
    int asciiValue;
    char character;
    int count;
    int initialPosition;
} CharInfo;

// Function to print character information
void printCharInfo(FILE* out, const CharInfo* charInfo) {
    fprintf(out, "Ascii Value: %d, Char: %c, Count: %d, Initial Position: %d\n",
            charInfo->asciiValue, charInfo->character, charInfo->count, charInfo->initialPosition);
}

// Function to perform character counting
void charCounter(FILE* in, FILE* out) {
    // Array to store information about each character (0-127)
    CharInfo charArray[128] = {0}; // Initialize all counts to zero

    // Variables to track total number of characters and unique characters
    int totalChars = 0;
    int uniqueChars = 0;

    // Read characters from the input file
    int ch;
    int position = 0;
    while ((ch = fgetc(in)) != EOF) {
        // Check if the character is in the valid ASCII range
        if (ch >= 0 && ch <= 127) {
            totalChars++;

            // Check if this is the first occurrence of the character
            if (charArray[ch].count == 0) {
                charArray[ch].asciiValue = ch;
                charArray[ch].character = (char)ch;
                charArray[ch].initialPosition = position;
                uniqueChars++;
            }

            // Increment the count for the character
            charArray[ch].count++;
        }
        else
        {
            printf("ERROR: Detecting Ascii Character %d at postion %d\n", ch, position);
        }
        

        position++;
    }

    // Print total number of characters and unique characters
    fprintf(out, "Total Number of Chars = %d\n", totalChars);
    fprintf(out, "Total Unique Chars = %d\n\n", uniqueChars);

    // Print information for each encountered character
    for (int i = 0; i < 128; i++) {
        if (charArray[i].count > 0) {
            printCharInfo(out, &charArray[i]);
        }
    }
    rewind(in);
}

// Function to create a new WORD node
WORD* createWordNode(const char* contents, int orderAppeared) {
    WORD* newWord = (WORD*)malloc(sizeof(WORD));
    if (newWord != NULL) {
        newWord->contents = strdup(contents);
        newWord->numChars = strlen(contents);
        newWord->frequency = 1;
        newWord->orderAppeared = orderAppeared;
        newWord->nextWord = NULL;
        newWord->prevWord = NULL;
    }
    return newWord;
}

// Function to free memory allocated for WORD nodes
void freeWordList(WORD* head) {
    while (head != NULL) {
        WORD* temp = head;
        head = head->nextWord;
        free(temp->contents);
        free(temp);
    }
}

// Function to print word information
void printWordInfo(FILE* out, const WORD* word) {
    fprintf(out, "Word: %s, Freq: %d, Initial Position: %d\n",
            word->contents, word->frequency, word->orderAppeared);
}

void insertSorted(WORD** head, WORD* newWord) {
    WORD* current = *head;
    WORD* prev = NULL;

    // Find the correct position to insert the new word
    while (current != NULL && strcmp(current->contents, newWord->contents) < 0) {
        prev = current;
        current = current->nextWord;
    }

    // Insert the new word
    if (prev == NULL) {
        newWord->nextWord = *head;
        *head = newWord;
    } else {
        prev->nextWord = newWord;
        newWord->nextWord = current;
    }
}

// Function to perform word counting
void wordCounter(FILE* in, FILE* out) {
    WORD* head = NULL; // Head of the linked list
    // WORD* tail = NULL; // Tail of the linked list
    int totalWords = 0;
    int uniqueWords = 0;
    int position = 0;

    char buffer[257];

    // Read words from the input file
    while (fscanf(in, "%256s", buffer) == 1) {
        totalWords++;

        // Check if this is the first occurrence of the word
        WORD* current = head;
        while (current != NULL) {
            if (strcmp(current->contents, buffer) == 0) {
                // Word already exists, increment frequency
                current->frequency++;
                break;
            }
            current = current->nextWord;
        }

        // If the word is not found, create a new node and add it to the linked list
        if (current == NULL) {
            WORD* newWord = createWordNode(buffer, position);
            if (newWord != NULL) {
                insertSorted(&head, newWord);
                uniqueWords++;
            }
        }

        position++;
    }

    // Print total number of words and unique words
    fprintf(out, "Total Number of Words: %d\n", totalWords);
    fprintf(out, "Total Unique Words: %d\n\n", uniqueWords);

    // Print information for each encountered word
    WORD* current = head;
    while (current != NULL) {
        printWordInfo(out, current);
        current = current->nextWord;
    }

    // Free memory allocated for WORD nodes
    freeWordList(head);
    rewind(in);
}

void printLineInfo(FILE* out, const WORD* line) {
    fprintf(out, "Line: %s, Freq: %d, Initial Position: %d\n",
            line->contents, line->frequency, line->orderAppeared);
}

void lineCounter(FILE* in, FILE* out){
    WORD* head = NULL; // Head of the linked list
    // WORD* tail = NULL; // Tail of the linked list
    int totalLines = 0;
    int uniqueLines = 0;
    int position = 0;

    char buffer[257];

    // Read words from the input file
    while (fgets(buffer, sizeof(buffer), in) != NULL) {
        totalLines++;
        buffer[strcspn(buffer, "\n")] = '\0';
        // Check if this is the first occurrence of the word
        WORD* current = head;
        while (current != NULL) {
            if (strcmp(current->contents, buffer) == 0) {
                // Word already exists, increment frequency
                current->frequency++;
                break;
            }
            current = current->nextWord;
        }

        // If the word is not found, create a new node and add it to the linked list
        if (current == NULL) {
            WORD* newWord = createWordNode(buffer, position);
            if (newWord != NULL) {
                insertSorted(&head, newWord);
                uniqueLines++;
            }
        }

        position++;
    }

    // Print total number of words and unique words 
    fprintf(out, "Total Number of Lines: %d\n", totalLines);
    fprintf(out, "Total Unique Lines: %d\n\n", uniqueLines);

    // Print information for each encountered word
    WORD* current = head;
    while (current != NULL) {
        printLineInfo(out, current);
        current = current->nextWord;
    }

    // Free memory allocated for WORD nodes
    freeWordList(head);
    rewind(in);
}

void longestLine(FILE* in, FILE* out){
    char buffer[257];
    char longestLine[257] = "";
    WORD* longestLinesHead = NULL; // Head of the linked list of longest lines
    int longestLength = 0;

    // Find the longest line length
    while (fgets(buffer, sizeof(buffer), in) != NULL) {
        int length = strlen(buffer);

        // Remove the newline character from the end of the line
        if (buffer[length - 1] == '\n') {
            buffer[length - 1] = '\0';
            length--;
        }

        if (length > longestLength) {
            longestLength = length;
            strcpy(longestLine, buffer);

            // Clear the longest line linked list
            freeWordList(longestLinesHead);
            longestLinesHead = NULL;
        } else if (length == longestLength) {
            // If the line is of the same length as the longest line, add it to the list
            WORD* newWord = createWordNode(buffer, 0);
            if (newWord != NULL) {
                insertSorted(&longestLinesHead, newWord);
            }
        }
    }

    // Print the longest line
    fprintf(out, "Longest Line is %d characters long:\n", longestLength);
    fprintf(out, "\t%s\n", longestLine);

    // Print all lines of the same length
    WORD* current = longestLinesHead;
    while (current != NULL) {
        fprintf(out, "\t%s\n", current->contents);
        WORD* temp = current;
        current = current->nextWord;
        free(temp->contents);
        free(temp);
    }
    rewind(in);
}

void longestWord(FILE* in, FILE* out){
    char buffer[257];
    WORD* longestWordsHead = NULL; // Head of the linked list of longest lines
    int longestLength = 0;

    // Find the longest line length
    while (fscanf(in, "%256s", buffer) == 1) {
        int length = strlen(buffer);

        // Remove the newline character from the end of the line
        if (buffer[length - 1] == '\n') {
            buffer[length - 1] = '\0';
            length--;
        }


        WORD* current = longestWordsHead;
        int found = 0;
        while (current != NULL) {
            if (strcmp(current->contents, buffer) == 0) {
                found = 1;
                break;
            }
            current = current->nextWord;
        }
        if (!found){
            if (length > longestLength) {
                longestLength = length;
                freeWordList(longestWordsHead);
                longestWordsHead = NULL;

                WORD* newWord = createWordNode(buffer, 0);
                if (newWord != NULL) {
                    insertSorted(&longestWordsHead, newWord);
                }

                // Clear the longest line linked list
            } else if (length == longestLength) {
                // If the line is of the same length as the longest line, add it to the list
                WORD* newWord = createWordNode(buffer, 0);
                if (newWord != NULL) {
                    insertSorted(&longestWordsHead, newWord);
                }
            }
        }
    }

    // Print the longest line
    fprintf(out, "Longest Word is %d characters long:\n", longestLength);

    // Print all lines of the same length
    WORD* current = longestWordsHead;
    while (current != NULL) {
        fprintf(out, "\t%s\n", current->contents);
        WORD* temp = current;
        current = current->nextWord;
        free(temp->contents);
        free(temp);
    }
    rewind(in);
}

int batchMode(FILE* batchFile){
    // Go to the end of the file, then check current position

    fseek (batchFile, 0, SEEK_END);
    int size = ftell(batchFile);
    if (size == 0)
    {
        printf("ERROR: Batch File Empty\n");
        return(1);
    }
    rewind(batchFile);

    char buffer[257];

    while (fgets(buffer, sizeof(buffer), batchFile) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        char* token = strtok(buffer, " ");
        // printf("TEST: %s\n", token);
        char *inputFile = NULL;
        char *outputFile = NULL;
        FILE* inputFP;
        FILE* outputFP = stdout;
        void (*functions[16])(FILE*, FILE*) = {NULL};
        int counter = 0;


        while (token != NULL)
        {
            // printf("%s\n", token);
            if (strcmp(token, "-f") == 0) {
                // printf("ARG!");
                token = strtok(NULL, " ");
                // printf("2: %s\n", token);
                if (token != NULL) {
                    inputFile = token;
                } else {
                    printf("Error: Missing input file after -f option.\n");
                    printUsage();
                    continue;
                }
            } else if (strcmp(token, "-o") == 0) {
                token = strtok(NULL, " ");
                if (token != NULL) {
                    outputFile = token;
                } else {
                    printf("Error: Missing output file after -o option.\n");
                    printUsage();
                    continue;
                }
            } else if (strcmp(token, "-c") == 0) {
                functions[counter++] = &charCounter;
            } else if (strcmp(token, "-w") == 0) {
                functions[counter++] = &wordCounter;
            } else if (strcmp(token, "-l") == 0) {
                functions[counter++] = &lineCounter;
            } else if (strcmp(token, "-Lw") == 0) {
                functions[counter++] = &longestWord;
            } else if (strcmp(token, "-Ll") == 0) {
                functions[counter++] = &longestLine;
            } else {
                printf("ERROR: Invalid Flag Types\n");
                continue;
            }
            token = strtok(NULL, " ");
        }
        if (inputFile == NULL || inputFile[0] == '-')
        {
            printf("ERROR: No Input File Provided\n");
            continue;
        }
        if (outputFile != NULL && outputFile[0] == '-')
        {
            printf("ERROR: No Output File Provided\n");
            continue;
        }
        
        // Open required files
        inputFP = fopen(inputFile, "r");
        if (inputFP == NULL)
        {
            printf("ERROR: Can't open input file\n");
            continue;
        } 
        fseek (inputFP, 0, SEEK_END);
        int size = ftell(inputFP);
        if (size == 0)
        {
            printf("ERROR: Input File Empty\n");
            continue;
        }
        rewind(inputFP);

        if (outputFile != NULL)
        {
            outputFP = fopen(outputFile, "w");

            if(outputFP == NULL)
            {
                printf("ERROR: Can't open output file\n");
                continue;
            }
        }
        for (int i = 0; i < counter; i++) {
            if (functions[i] != NULL) {
                functions[i](inputFP, outputFP);
            }
            if (i <= counter-2)
            {
                fprintf(outputFP, "\n");
            }
        }
        inputFile = NULL; inputFP = NULL; outputFile = NULL; outputFP = stdout;   

    }
        
        
    
    return(0);
}

int main(int argc, char *argv[]) {
    // Check if the minimum number of arguments is met
    if (argc < 3) {
        printUsage();
        return 1; // Return error code
    }

    // Initialize optional flags with default values
    char *inputFile = NULL;
    char *outputFile = NULL;
    char *batchFile = NULL;
    FILE* inputFP;
    FILE* outputFP = stdout;


    // Loop through command line arguments
    for (int i = 1; i < argc; i++) {
        // Check for required options
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 < argc) {
                inputFile = argv[i + 1];
                i++; // Skip the next argument (input file)
            } else {
                printf("Error: Missing input file after -f option.\n");
                printUsage();
                return(1);
            }
        } else if (strcmp(argv[i], "-B") == 0) {
            if (i + 1 < argc) {
                batchFile = argv[i + 1];
                FILE* batchFP = fopen(batchFile, "r");
                if (batchFP == NULL)
                {
                    printf("ERROR: Can't open batch file\n");
                    return(1);
                }
                return batchMode(batchFP);
                
                i++; // Skip the next argument (batch file)
            } else {
                printf("Error: Missing batch file after -B option.\n");
                printUsage();
                return(1);
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                outputFile = argv[i + 1];
                i++; // Skip the next argument (output file)
            } else {
                printf("Error: Missing output file after -o option.\n");
                printUsage();
                return(1);
            }
        }
    }
    if (inputFile == NULL || inputFile[0] == '-')
    {
        printf("ERROR: No Input File Provided\n");
        return(1);
    }
    if (outputFile != NULL && outputFile[0] == '-')
    {
        printf("ERROR: No Output File Provided\n");
        return(1);
    }
    
    // Open required files
    inputFP = fopen(inputFile, "r");
    if (inputFP == NULL)
    {
        printf("ERROR: Can't open input file\n");
        return(1);
    } 
    fseek (inputFP, 0, SEEK_END);
    int size = ftell(inputFP);
    if (size == 0)
    {
        printf("ERROR: Input File Empty\n");
        return(1);
    }
    rewind(inputFP);

    if (outputFile != NULL)
    {
        outputFP = fopen(outputFile, "w");

        if(outputFP == NULL)
        {
            printf("ERROR: Can't open output file\n");
            return(1);
        }
    }
    
    // Note 16 is an large enough number
    void (*functions[16])(FILE*, FILE*) = {NULL};
    int counter = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            functions[counter++] = &charCounter;
        } else if (strcmp(argv[i], "-w") == 0) {
            functions[counter++] = &wordCounter;
        } else if (strcmp(argv[i], "-l") == 0) {
            functions[counter++] = &lineCounter;
        } else if (strcmp(argv[i], "-Lw") == 0) {
            functions[counter++] = &longestWord;
        } else if (strcmp(argv[i], "-Ll") == 0) {
            functions[counter++] = &longestLine;
        } else if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "-B") == 0) || (strcmp(argv[i], "-f") == 0)) {
            // Skip the next argument for -o, -B, -f flags
            i++;
        } else {
            printf("ERROR: Invalid Flag Types\n");
            return 1;
        }
    }

    // Execute functions in the order they appear in the command arguments
    for (int i = 0; i < counter; i++) {
        if (functions[i] != NULL) {
            functions[i](inputFP, outputFP);
        }
        if (i <= counter-2)
        {
            fprintf(outputFP, "\n");
        }
        
    }

    // Close all files and finish clean up
    fclose(inputFP);
    if (outputFP != stdout)
    {
        fclose(outputFP);
    }
    
    return 0; // Return success code
}

