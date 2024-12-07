/*
Scenario: Student Grade Management System with Process Creation.
You are tasked with creating a student grade management system in C programming language.
The system should allow the user to manage student grades stored in a file.
The user should be able to perform the following operations:
*/

//the aim of program is to using fork() function to create a child process and parent process
//and then using exec() function to run the child process and parent process.
//and using properly open system calls  
//and using properly close system calls

//project description:
/*
You are tasked with creating a student grade management system in Cprogramming language.
The system should allow the user to manage student grades stored in a file.
The user should be able to perform the following operations: 
gtuStudentGrades “grades.txt” 
should create a file.
Use fork() system call for process creation.*/

/*
1. Add Student Grade:The user should be able to add a new student's grade to the system.
The program should prompt the user to enter the student's name and grade,
and then add this information to the file using a separate process for file manipulation.
addStudentGrade “Name Surname” “AA” 
command should append student and grade to the end of the file.
*/

/*
2. Search for Student Grade:The user should be able to search for a student's grade by entering the student's name.
The program should then display the student's name and grade if it exists in the file.
searchStudent “Name Surname” 
command should return student name surname and grade.
*/

/*
3. Sort Student Grades:The user should be able to sort the student grades in the file.
The program should provide options to sort by student name or grade, in ascending or descending order.
sortAll “gradest.txt”
command should print all of the entries sorted by their names.
*/

/*
. Display Student Grades:The user should be able to display all student grades stored in the file. 
The program should display the student name and grade for each student.
Also display content page by page and first 5 entries.
showAll “grades.txt” 
command should print all of the entries in the file.
listGrades “grades.txt” 
command should print first 5 entries.
listSome “numofEntries” “pageNumber” “grades.txt” 
e.g. listSome 5 2 grades.txt command will list entries between 5th and 10th.
*/

/*
5. Logging: Create a log file that records the completion of each task as desired.
*/

/*
6. Usage: The user should be ableto display all of the available commands by calling gtuStudentGrades without an argument.
gtuStudentGrades command without arguments should list all of the instructions or information provided about how commands in your program.
*/

/*
For testing the assignment, consider a scenario where you have a file `grades.txt`
that contains student grades in the format "name, grade" (e.g., "Name Surname, AA").
You can test adding, searching, sorting, and displaying student grades using this file. 
Ensure that the program handles errors gracefully and uses process creation (fork) to manage file operations concurrently.*/


// Required Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

//function to write to file
void writeToFile(char *fileName, char *content){
    int fileDescriptor = open(fileName, O_WRONLY | O_APPEND | O_CREAT, 0644); //open the file or create a new one
    if(fileDescriptor == -1){ //if the file does not exist, give error
        perror("Error opening file"); //print the error message to terminal
        exit(1); //exit the program
    }
    write(fileDescriptor, content, strlen(content)); //write the content to the file
    close(fileDescriptor); //close the file
}

//function to read from file
char *readFromFile(char *fileName){
    int fileDescriptor = open(fileName, O_RDONLY); //open the file with read only mode
    if(fileDescriptor == -1){ //if the file does not exist, give error
        perror("Error opening file"); //print the error message to terminal
        exit(1); //exit the program
    }
    int fileSize = lseek(fileDescriptor, 0, SEEK_END); //find the size of the file
    lseek(fileDescriptor, 0, SEEK_SET); //set the file pointer to the beginning of the file
    char *content = (char *)malloc(fileSize * sizeof(char)); //allocate memory for the content of the file
    read(fileDescriptor, content, fileSize); //read the content of the file
    close(fileDescriptor); //close the file
    return content;
}

//function to write to log file
void writeLog(char *command, char *message){
    int fileDescriptor = open("log.txt", O_WRONLY | O_APPEND | O_CREAT, 0644); //open the log file or create a new one
    if(fileDescriptor == -1){ //if the file does not exist, give error
        perror("Error opening log file"); //print the error message to terminal
        exit(1);
    }
    write(fileDescriptor, command, strlen(command)); //write the command to the log file
    write(fileDescriptor, " ", 1);
    write(fileDescriptor, message, strlen(message)); //write the message to the log file
    write(fileDescriptor, "\n", 1);
    close(fileDescriptor);
}

//addStudentGrade function. the argumants are name, grade, and file name.
void addStudentGrade(char *name, char *grade, char *fileName){
    

    pid_t pid = fork(); //create a child process
    if(pid == -1){ //if the child process is not created, give error
        perror("Error creating child process"); //print the error message to terminal
        writeLog("addStudentGrade", "Error creating child process"); //write to log file
        return;
    }
   else if(pid > 0){
        int status;
        wait(&status); //wait for the child process to finish
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0){ //if the child process is finished successfully
            writeLog("addStudentGrade", "Student grade added successfully"); //write to log file 
        }
        else{
            writeLog("addStudentGrade", "Error adding student grade"); //write to log file  
        }
    }
    else{
        //grade can only be 2 character. e.g.: AA, BA, BB, CB, CC, DC, DD, FD, FF, NA, VF
        if(strlen(grade) != 2){
        writeLog("addStudentGrade", "Invalid grade format"); //write to log file that the grade format is invalid
        write(1, "Invalid grade format\n", 21); //print the error message to terminal
        exit(0); //exit the child process
        }
        writeToFile(fileName, name); //write the student name to the file
        writeToFile(fileName, ", ");
        writeToFile(fileName, grade); //write the student grade to the file
        writeToFile(fileName, "\n");
        exit(0); //exit the child process
   }
    
}

//searchStudent function. the argumants are name and file name.
void searchStudent(char *name, char *fileName){
    pid_t pid = fork(); //create a child process
    if(pid == -1){ //if the child process is not created, give error
        perror("Error creating child process"); //print the error message to terminal
        writeLog("searchStudent", "Error creating child process"); //write to log file
        return;
    }
    else if(pid > 0){
        int status;
        wait(&status); //wait for the child process to finish
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0){ //if the child process is finished successfully
            writeLog("searchStudent", "Student grade searched successfully"); //write to log file
        }
        else{
            writeLog("searchStudent", "Student can not found in the file"); //write to log file
        }
    }
    else{
        char *content = readFromFile(fileName); // Dosya içeriğini oku
        char *line = strtok(content, "\n"); // Dosya içeriğini satır satır ayır
        int found = 0;
        while(line != NULL){ // Dosyayı satır satır ara
            if(strstr(line, name) != NULL){ // Eğer öğrenci adı bulunursa
                write(1, line, strlen(line)); // Öğrenci adını ve notunu yazdır
                write(1, "\n", 1);
                found = 1;
                break;
            }
            line = strtok(NULL, "\n"); // Sonraki satıra geç
        }
        if(!found) {
            write(1, "Student cannot be found in the file\n", 36);
        }
        free(content); // Dinamik olarak ayrılan belleği serbest bırak
        exit(found ? 0 : 1); // Öğrenci bulunursa 0, bulunamazsa 1 ile çık
   }
}

//function for count the number of lines in the file
int countLines(char *fileName){
    int fileDescriptor = open(fileName, O_RDONLY); //open the file with read only mode
    if(fileDescriptor == -1){
        perror("Error opening file");
        exit(1);
    }
    int count = 0;
    char buffer; //buffer to read the file
    while(read(fileDescriptor, &buffer, 1) > 0){ //read the file character by character
        if(buffer == '\n'){ //if the character is new line character
            count++; //increment the count
        }
    }
    close(fileDescriptor); //close the file
    return count; //return the number of lines
}

//compare the names for sorting
int compareNames(const void *a, const void *b){ //compare the names for sorting
    char *name1 = *(char **)a; //get the first name
    char *name2 = *(char **)b; //get the second name
    return strcmp(name1, name2); //compare the names and return the result of the comparison if the first name is smaller than the second name, return -1, if the first name is greater than the second name, return 1, if the names are equal, return 0
}

//compare the grades for sorting
//examples of line content in the file: "Matthew Williams, BB"
int compareGradesAsc(const void *a, const void *b) { //compare the grades for sorting in ascending order
    const char *gradeA = strrchr(*(const char **)a, ',') + 2; //get the grade of the first student 
    const char *gradeB = strrchr(*(const char **)b, ',') + 2; //get the grade of the second student
    return strcmp(gradeA, gradeB); //compare the grades and return the result of the comparison if the first grade is smaller than the second grade, return -1, if the first grade is greater than the second grade, return 1, if the grades are equal, return 0
}

//compare the grades for sorting in descending order
int compareGradesDesc(const void *a, const void *b) {
    return -compareGradesAsc(a, b); //return the negative of the comparison result of the grades for sorting in descending order
}

//compare the names for sorting in ascending order
int compareNamesAsc(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b); //compare the names for sorting in ascending order
}

//compare the names for sorting in descending order
int compareNamesDesc(const void *a, const void *b) {
    return -compareNamesAsc(a, b); //return the negative of the comparison result of the names for sorting in descending order
}

//sortAll function. the argumants are file name, option1, and option2.
void sortAll(char *fileName, char *option1, char *option2) {
  pid_t pid = fork(); //create a child process
    if(pid == -1){ //if the child process is not created, give error
        perror("Error creating child process"); //print the error message to terminal
        writeLog("sortAll", "Error creating child process"); //write to log file
        return;
    }
    else if(pid > 0){
        int status;
        wait(&status); //wait for the child process to finish
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0){ //if the child process is finished successfully
            writeLog("sortAll", "Student grades sorted successfully"); //write to log file
        }
        else{
            writeLog("sortAll", "Error sorting student grades"); //write to log file
        }
    }
    else{
        int file_descriptor = open(fileName, O_RDONLY); //open the file with read only mode
        if (!file_descriptor) { //if the file does not exist, give error
            perror("Error opening file");
            writeLog("sortAll", "File cannot be read or does not exist");
            exit(1);

        }

        int lineCount = countLines(fileName); //count the number of lines in the file
        if (lineCount <= 0) {
            write(1, "File is empty or cannot be read.\n", 33); //print the error message to terminal
            writeLog("sortAll", "File is empty or cannot be read");
            exit(1);
        }

        // Allocate memory for the lines to be read from the file
        char **lines = malloc(lineCount * sizeof(char *)); //allocate memory for the lines to be read from the file
        char buffer[256]; //buffer to read the file.
        char tempLine[256]; //buffer to store the line
        int bytesRead, tempLineIndex = 0; //variables to store the number of bytes read and the index of the temporary line

        for (int i = 0; i < lineCount; i++) { //read the file line by line
            tempLineIndex = 0; //initialize the index of the temporary line
            while ((bytesRead = read(file_descriptor, buffer, 1)) > 0) { //read the file character by character

                if (buffer[0] == '\n') {  // check if the character is new line character
                    tempLine[tempLineIndex] = '\0'; // add null character to the end of the line
                    lines[i] = strdup(tempLine); // copy the line to the lines array
                    break;
                } else {
                    tempLine[tempLineIndex++] = buffer[0]; // add the character to the temporary line
                }
            }
            if (bytesRead <= 0) { // if the number of bytes read is less than or equal to 0. break the loop
                break;
            }
        }
        close(file_descriptor);

        // Sort the lines based on the options
        if (strcmp(option1, "name") == 0) { //if the option is name
            qsort(lines, lineCount, sizeof(char *), strcmp(option2, "ascending") == 0 ? compareNamesAsc : compareNamesDesc); //sort the lines based on the names in ascending or descending order
        } else if (strcmp(option1, "grade") == 0) { //if the option is grade
            qsort(lines, lineCount, sizeof(char *), strcmp(option2, "ascending") == 0 ? compareGradesAsc : compareGradesDesc); //sort the lines based on the grades in ascending or descending order
        }

        for (int i = 0; i < lineCount; i++) { //print the sorted lines
            write(1, lines[i], strlen(lines[i]));
            write(1, "\n", 1);
            free(lines[i]); //free the memory allocated for the line
        }
        free(lines); // free the memory allocated for the lines array
        exit(0); //exit the child process
    }
}

//showAll function. the argumants are file name.
void showAll(char *fileName){
    pid_t pid = fork(); //create a child process
    if(pid == -1){ //if the child process is not created, give error
        perror("Error creating child process"); //print the error message to terminal
        writeLog("showAll", "Error creating child process"); //write to log file
        return;
    }
    else if(pid > 0){
        int status;
        wait(&status); //wait for the child process to finish
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0){ //if the child process is finished successfully
            writeLog("showAll", "All student grades displayed successfully"); //write to log file
        }
        else{
            writeLog("showAll", "Error displaying student grades"); //write to log file
        }
    }
    else{
        int file_descriptor = open(fileName, O_RDONLY); //open the file with read only mode
        if (!file_descriptor) { //if the file does not exist, give error
            perror("Error opening file");
            writeLog("showAll", "File cannot be read or does not exist");
            exit(1);
        }

        int readBytes;
        char buffer[1024]; //buffer to read the file
        while ((readBytes = read(file_descriptor, buffer, 1024)) > 0) { //read the file character by character
            write(1, buffer, readBytes); //print the content of the file
        }
        close(file_descriptor); //close the file
        exit(0); //exit the child process
    }
    
}


//listGrades function to print to terminal the first 5 entries(lines) in the file
void listGrades(char *fileName)
{
    pid_t pid = fork(); //create a child process
    if(pid == -1){ //if the child process is not created, give error
        perror("Error creating child process"); //print the error message to terminal
        writeLog("listGrades", "Error creating child process"); //write to log file
        return;
    }
    else if(pid > 0){
        int status;
        wait(&status); //wait for the child process to finish
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0){ //if the child process is finished successfully
            writeLog("listGrades", "First 5 student grades displayed successfully"); //write to log file
        }
        else{
            writeLog("listGrades", "Error displaying first 5 student grades"); //write to log file
        }
    }
    else{
        char *content = readFromFile(fileName);
        //if the number of lines in the file is less than 5, print all the lines and give an error message to the terminal
        char *line = strtok(content, "\n"); //split the content of the file by new line character
        int countofLines = countLines(fileName);
        if(countofLines < 5){ //if the number of lines in the file is less than 5
            write(1, "Less than 5 student grades in the file\n", 38);
            writeLog("listGrades", "Less than 5 student grades in the file");
            for (int i = 0; i < countofLines; i++) 
            {
                write(1, line, strlen(line));
                write(1, "\n", 1);
                line = strtok(NULL, "\n");
            }
            exit(0);
        }
        else{
            for (int i = 0; i < 5; i++) //print the first 5 lines
            {
                write(1, line, strlen(line));
                write(1, "\n", 1);
                line = strtok(NULL, "\n");
            }
            //write to log file
            writeLog("listGrades", "First 5 student grades displayed successfully");
            exit(0);
        } 
    }  
}

//listSome function to print to terminal the entries between the given page number and number of entries
void listSome(int numofEntries, int pageNumber, char *fileName){
    pid_t pid = fork(); //create a child process
    if(pid == -1){ //if the child process is not created, give error
        perror("Error creating child process"); //print the error message to terminal
        writeLog("listSome", "Error creating child process"); //write to log file
        return;
    }
    else if(pid > 0){
        int status;
        wait(&status); //wait for the child process to finish
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0){ //if the child process is finished successfully
            writeLog("listSome", "Student grades displayed successfully"); //write to log file
        }
        else{
            writeLog("listSome", "Error displaying student grades"); //write to log file
        }
    }
    else{
        //If the desired range is not in the file, that is, if the number of lines is less than the desired range, give an error.
        int countofLines = countLines(fileName); //count the number of lines in the file
        if(countofLines < numofEntries * pageNumber){ //if the number of lines in the file is less than the desired range
            write(1, "The desired range is not in the file\n", 37); //print the error message to terminal
            writeLog("listSome", "The desired range is not in the file"); //write to log file
            exit(0);
        }
        char *content = readFromFile(fileName); //read the content of the file
        char *line = strtok(content, "\n"); //split the content of the file by new line character
        int count = 0; //initialize the count
        int start = (pageNumber - 1) * numofEntries; //calculate the start index
        int end = pageNumber * numofEntries; //calculate the end index
        while(line != NULL){ //search the file line by line
            if(count >= start && count < end){ //if the line is between the start and end index
                write(1, line, strlen(line)); //print the line
                write(1, "\n", 1); 
            }
            count++; //increment the count
            line = strtok(NULL, "\n"); //get the next line
        }
        writeLog("listSome", "Some student grades displayed successfully");
        exit(0);
    }
}
//main function
//when the program start running user can enter the different commands. 
//if the user enters the command without any argument, the program should display the available commands.
//if the user enters the command with arguments, the program should execute the command.
//program shlould run in a loop until the user enters the exit command.
//exit command like this: if you want to exit the program, press q button.
//wait for the user to enter the command

int main(int argc, char *argv[]){
    char *command = (char *)malloc(1024 * sizeof(char));
    char *name = (char *)malloc(1024 * sizeof(char));
    char *grade = (char *)malloc(1024 * sizeof(char));
    char *content = (char *)malloc(1024 * sizeof(char));
    while(1){
        write(1, "Enter a command: ", 17);
        fgets(command, 1024, stdin);
        if(strncmp(command, "q",1) == 0){
            free(command);
            free(name);
            free(grade);
            free(content);
            break;
        }
        if(strncmp(command, "gtuStudentGrades",16 ) == 0){
            //if the user enters the command without any argument, the program should display the available commands.
            //if the user enters the command with arguments, the program should execute the command.
            //gtuStudentGrades "fileName.txt" command should create a file.

            char *fileName = (char *)malloc(1024 * sizeof(char));
            if(sscanf(command, "gtuStudentGrades \"%[^\"]\"", fileName) == 1){
                int fileDescriptor = open(fileName, O_WRONLY | O_APPEND | O_CREAT, 0644); //open the file or create a new one
                if(fileDescriptor == -1){ //if the file does not exist, give error
                    perror("Error opening file"); //print the error message to terminal
                    writeLog("gtuStudentGrades", "Error opening file"); //write to log file
                    free(fileName); //free malloc
                    continue;
                }
                close(fileDescriptor); //close the file
                writeLog("gtuStudentGrades", "File created successfully"); //write to log file
            }
            else if(sscanf(command, "gtuStudentGrades") == 0){
                write(1, "Usage:\n", 7);
                write(1, "addStudentGrade \"Name Surname\" \"Grade\" \"fileName.txt\"\n", 55);
                write(1, "searchStudent \"Name Surname\" \"fileName.txt\"\n", 45);
                write(1, "sortAll \"fileName.txt\"\n", 24);
                write(1, "sortAll \"Name\" \"ascending or descending\" \"fileName.txt\"\n", 57);
                write(1, "sortAll \"Grade\" \"ascending or descending\" \"fileName.txt\"\n", 58);
                write(1, "showAll \"fileName.txt\"\n", 24);
                write(1, "listGrades \"fileName.txt\"\n", 27);
                write(1, "listSome numofEntries pageNumber \"fileName.txt\"\n", 48);
                write(1, "q\n", 2);
                writeLog("gtuStudentGrades", "Usage displayed");
            }
            else{
                write(1, "Invalid command\n", 16);
                writeLog("gtuStudentGrades", "Invalid command");

            }
            //free malloc
            free(fileName);
        }
        else if(strncmp(command, "addStudentGrade", 15) == 0){
            char *fileName = (char *)malloc(1024 * sizeof(char));
            //file name is given by user as an argument like this: addStudentGrade "Name Surname" "AA" "grades.txt"
            // 
            //instead of using sscanf function, use read function to get the name, grade, and file name from the user
            //name: "Name Surname"
            //grade: "AA"
            //file name: "grades.txt"
            //user use the "" sign to separate the name, grade, and file name
            //use read function to get the name, grade, and file name from the user

            sscanf(command, "addStudentGrade \"%[^\"]\" \"%[^\"]\" \"%[^\"]\"", name, grade, fileName);
            addStudentGrade(name, grade, fileName);
            //free malloc
            free(fileName);
        }
        else if(strncmp(command, "searchStudent", 13) == 0){
            //file name is given by user as an argument like this: searchStudent "Name Surname" "grades.txt"
            char *fileName = (char *)malloc(1024 * sizeof(char));
            sscanf(command, "searchStudent \"%[^\"]\" \"%[^\"]\"", name, fileName);
            searchStudent(name, fileName);
            //free malloc
            free(fileName);
        }
        else if(strncmp(command, "sortAll", 7) == 0){ 
            char *fileName = (char *)malloc(1024 * sizeof(char));
            char *option1 = (char *)malloc(1024 * sizeof(char));
            char *option2 = (char *)malloc(1024 * sizeof(char));
            //scanf the all command and arguments from the user in a single line and count the number of arguments

            if(sscanf(command, "sortAll \"%[^\"]\" \"%[^\"]\" \"%[^\"]\"", option1, option2, fileName) == 3){ 
                //give an error if option1 is not equal to "name" or "grade"
                if(strcmp(option1, "name") != 0 && strcmp(option1, "grade") != 0){
                    write(1, "Invalid option1\n", 17);
                    writeLog("sortAll", "Invalid sort option");
                    free(fileName);
                    free(option1);
                    free(option2);
                    continue;
                }
                sortAll(fileName, option1, option2);
            }
            else if(sscanf(command, "sortAll \"%[^\"]\"", fileName) == 1){
                sortAll(fileName, "name", "ascending");
            }
            else{
                write(1, "Invalid command\n", 16);
                writeLog("sortAll", "Invalid command");
            }
            //free malloc
            free(fileName);
            free(option1);
            free(option2);

        }
        else if(strncmp(command, "showAll", 7) == 0){
            char *fileName = (char *)malloc(1024 * sizeof(char));
            sscanf(command, "showAll \"%[^\"]\"", fileName);
            //use showAll function to print the content of the file
            showAll(fileName);
            //free malloc
            free(fileName);
            
        }
        else if(strncmp(command, "listGrades", 10) == 0){
            char *fileName = (char *)malloc(1024 * sizeof(char));
            sscanf(command, "listGrades \"%[^\"]\"", fileName);
            //use listGrades function to print the first 5 entries in the file
            listGrades(fileName);
            //free malloc
            free(fileName);
        }
        else if(strncmp(command, "listSome", 8) == 0){
            char *fileName = (char *)malloc(1024 * sizeof(char));
            int numofEntries;
            int pageNumber;
            sscanf(command, "listSome %d %d \"%[^\"]\"", &numofEntries, &pageNumber, fileName);
            //use listSome function to print the entries between the given page number and number of entries
            listSome(numofEntries, pageNumber, fileName);
            //free malloc
            free(fileName);
        }
        else{
            write(1, "Invalid command\n", 16);
            writeLog("Invalid command", "Invalid command");
        }
    }
    return 0;
}