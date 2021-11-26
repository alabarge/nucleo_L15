#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <sys/types.h>

// Windows does not define the S_ISREG and S_ISDIR macros in stat.h, so we do.
// We have to define _CRT_INTERNAL_NONSTDC_NAMES 1 before #including sys/stat.h
// in order for Microsoft's stat.h to define names like S_IFMT, S_IFREG, and S_IFDIR,
// rather than just defining  _S_IFMT, _S_IFREG, and _S_IFDIR as it normally does.
#define _CRT_INTERNAL_NONSTDC_NAMES 1
#include <sys/stat.h>
#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
  #define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISDIR) && defined(S_IFMT) && defined(S_IFDIR)
  #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

int main(int argc, char *argv[]) {

   FILE  *ver, *git;

   struct tm *now;
   time_t clock;

   uint32_t build_major = 0;
   uint32_t build_minor = 0;
   uint32_t build_inc   = 0;
   uint32_t build_num   = 0;
   uint32_t build_hex   = 0;
   uint32_t build_sysid = 0;
   uint32_t error       = 0;

   uint32_t build_inc_found = 0;
   uint32_t j, ret;

   char  build_user[128]   = {0};
   char  build_time[128]   =  {0};
   char  build_date[128]   = {0};
   char  build_hi[128]     = {0};
   char  build_lo[128]     = {0};
   char  build_str1[128]   = {0};
   char  build_str2[128]   = {0};
   char  in_line[1024]     = {0};
   char  git_rev[128]      = {0};
   char  git_auth[128]     = {0};
   char  git_email[128]    = {0};
   char  git_date[128]     = {0};

   char  *env, *token;

   char  cwd[128];

   char  *month[] = {
            "JAN", "FEB", "MAR", "APR",
            "MAY", "JUN", "JUL", "AUG",
            "SEP", "OCT", "NOV", "DEC"
          };

   printf("\nFirmware Version Utility 1.10\n");

   // command Line
   printf("cmd : ");
   for (j=0;j<argc;j++) {
      printf("%s ", argv[j]);
   }
   printf("\n");

   // current directory
   if (getcwd(cwd, sizeof(cwd)) != 0) printf("cwd : %s\n", cwd);

   if (argc < 5) {
      printf("Error: Filenames not specified.\n");
      return -1;
   }

   //
   // If build.h exists then parse for BUILD_INC
   //
   if  ((ver = fopen(argv[2],"rt")) != NULL) {
      while (fgets(in_line, sizeof(in_line), ver) != NULL) {
         token = strtok(in_line, " ");
         token = strtok(NULL, " ");
         if (token == NULL) continue;
         if (strcmp(token, "BUILD_INC") == 0) {
            token = strtok(NULL, "\n");
            sscanf(token, "%d", &build_inc);
            build_inc &= 0x00FF;
            build_inc_found = 1;
            break;
         }
      }
      fclose(ver);
   }

   //
   // Parse build.inc
   //
   if  ((ver = fopen(argv[1],"rt")) != NULL) {
      if (fgets(in_line, sizeof(in_line), ver) != NULL) {
         token = strtok(in_line, " ");
         token = strtok(NULL, " ");
         if (strcmp(token, "BUILD_MAJOR") == 0) {
            token = strtok(NULL, "\n");
            sscanf(token, "%d", &build_major);
            build_major &= 0x00FF;
         }
         else {
            error = 1;
            goto ERROR;
         }
      }
      else {
         error = 1;
         goto ERROR;
      }
      if (fgets(in_line, sizeof(in_line), ver) != NULL) {
         token = strtok(in_line, " ");
         token = strtok(NULL, " ");
         if (strcmp(token, "BUILD_MINOR") == 0) {
            token = strtok(NULL, "\n");
            sscanf(token, "%d", &build_minor);
            build_minor &= 0x00FF;
         }
         else {
            error = 2;
            goto ERROR;
         }
      }
      else {
         error = 3;
         goto ERROR;
      }
      if (fgets(in_line, sizeof(in_line), ver) != NULL) {
         token = strtok(in_line, " ");
         token = strtok(NULL, " ");
         if (strcmp(token, "BUILD_NUM") == 0) {
            token = strtok(NULL, "\n");
            sscanf(token, "%d", &build_num);
            build_num &= 0x00FF;
         }
         else {
            error = 4;
            goto ERROR;
         }
      }
      else {
         error = 5;
         goto ERROR;
      }
      if (fgets(in_line, sizeof(in_line), ver) != NULL) {
         token = strtok(in_line, " ");
         token = strtok(NULL, " ");
         if (strcmp(token, "BUILD_INC") == 0) {
            token = strtok(NULL, "\n");
            // If build_inc not found in build.h
            if (build_inc_found == 0) {
               sscanf(token, "%d", &build_inc);
               build_inc &= 0x00FF;
            }
         }
         else {
            error = 6;
            goto ERROR;
         }
      }
      else {
         error = 7;
         goto ERROR;
      }
      if (fgets(in_line, sizeof(in_line), ver) != NULL) {
         token = strtok(in_line, " ");
         token = strtok(NULL, " ");
         if (strcmp(token, "BUILD_SYSID") == 0) {
            token = strtok(NULL, "\n");
            sscanf(token, "%x", &build_sysid);
         }
         else {
            error = 8;
            goto ERROR;
         }
      }
      else {
         error = 9;
         goto ERROR;
      }
      fclose(ver);
   }

   ERROR:

   if (error == 1) {
      fclose(ver);
      printf("Error Parsing %s, %d\n", argv[1], error);
      return -1;
   }

   // Increment Build Number, only 0 to 255
   build_inc++;
   build_inc &= 0x00FF;

   // 32-Bit Build Number
   build_hex = (build_major << 24) | (build_minor << 16) | (build_num << 8) | build_inc;

   // Open File for Writing
   if  ((ver = fopen(argv[2],"wt")) == NULL) {
      printf("Error: Unable to open %s for writing\n", argv[2]);
      exit (-1);
   }

   // Get Current Time & Date
   time(&clock);
   now = localtime(&clock);
   sprintf(build_time,"%02d:%02d", now->tm_hour, now->tm_min);
   sprintf(build_date,"%02d.%s.%02d", now->tm_mday, month[now->tm_mon],
         (now->tm_year+1900)-2000);

   // Get Current User
   memset(build_user, 0, sizeof(build_user));
   env = getenv("USERNAME");
   if (env != NULL) {
      strcpy(build_user, env);
   }
   else {
      build_user[0] = '\0';
   }

   remove("git_hash.txt");
   remove("git_porcelain.txt");

   // Check for .git existence
   struct stat status;
   stat(argv[4], &status);

   // If .git directory then gather state info
   if (S_ISDIR(status.st_mode)) {

      ret = system("git show --quiet HEAD > git_hash.txt");
      ret = system("git status --porcelain > git_porcelain.txt");

      // Get git HEAD detail
      //status --porcelain
      // Parse git_hash.txt
      //
      if ((git = fopen("git_hash.txt","rt")) != NULL) {
         while (fgets(in_line, sizeof(in_line), git) != NULL) {
            // ignore comments
            if (in_line[0] == '#') continue;
            // ignore blank lines
            if (in_line[0] == '\n') continue;
            // ignore blank lines
            if (in_line[0] == '\r') continue;
            // ignore lines that begin with a space
            if (in_line[0] == ' ') continue;
            // extract key
            token = strtok(in_line, " ");
            // commit
            if (strcmp(token, "commit") == 0) {
               token = strtok(NULL, " ");
               sprintf(git_rev, "%s", token);
               // last sixteen characters of hash
               memcpy(git_rev, &git_rev[24], 16);
               git_rev[16] = 0x00;
            }
            // Merge
            if (strcmp(token, "Merge:") == 0) {
               token = strtok(NULL, " ");
            }
            // Author
            if (strcmp(token, "Author:") == 0) {
               token = strtok(NULL, "<");
               sprintf(git_auth, "%s", token);
               token = strtok(NULL, ">");
               sprintf(git_email, "%s", token);
            }
            // Date
            if (strcmp(token, "Date:") == 0) {
               token = strtok(NULL, "\n");
               sprintf(git_date, "%s", token);
            }
         }
         fclose(git);
         remove("git_hash.txt");
      }

      // Get git status detail
      //
      // Parse git_porcelain.txt
      //
      if  ((git = fopen("git_porcelain.txt","rt")) != NULL) {
         while (fgets(in_line, sizeof(in_line), git) != NULL) {
            token = strtok(in_line, " ");
            if ((strcmp(token, "M") == 0) ||
                (strcmp(token, "A") == 0) ||
                (strcmp(token, "D") == 0) ||
                (strcmp(token, "R") == 0) ||
                (strcmp(token, "C") == 0)) {
               strcat(git_rev, "*");
               break;
            }
         }
         fclose(git);
         remove("git_porcelain.txt");
      }
   }
   else {
      printf("Warning : The %s Directory was not found\n", argv[4]);
   }

   // Create Build Strings
   sprintf(build_lo, "%d.%d.%d build %d", build_major, build_minor, build_num, build_inc);
   sprintf(build_hi, "%s, %s %s 0x%08X [%s] %s", build_lo, build_time, build_date, build_sysid, build_user, git_rev);
   sprintf(build_str1, "%s (%04d/%02d/%02d)", "", now->tm_year+1900, now->tm_mon+1, now->tm_mday);
   sprintf(build_str2, "%d.%d.%d.%d", build_major, build_minor, build_num, build_inc);

   fprintf(ver, "#pragma once\n\n");

   fprintf(ver, "#define FPGA_MAJOR      %d\n", build_major);
   fprintf(ver, "#define FPGA_MINOR      %d\n", build_minor);
   fprintf(ver, "#define FPGA_NUM        %d\n", build_num);
   fprintf(ver, "#define FPGA_INC        %d\n", build_inc);
   fprintf(ver, "#define FPGA_SYSID      0x%08X\n", build_sysid);
   fprintf(ver, "#define FPGA_VER_HEX    0x%08X\n", build_hex);
   fprintf(ver, "#define FPGA_TIME       \"%s\"\n", build_time);
   fprintf(ver, "#define FPGA_DATE       \"%s\"\n", build_date);
   fprintf(ver, "#define FPGA_USER       \"%s\"\n", build_user);
   fprintf(ver, "#define FPGA_STR        \"%s\"\n", build_str2);
   fprintf(ver, "#define FPGA_LO         \"%s\"\n", build_lo);
   fprintf(ver, "#define FPGA_HI         \"%s\"\n", build_hi);
   fprintf(ver, "#define FPGA_STRING     \"%s\"\n", build_str2);
   fprintf(ver, "#define FPGA_EPOCH      %d\n", (uint32_t)clock);
   fprintf(ver, "#define FPGA_EPOCH_HEX  0x%08X\n", (uint32_t)clock);
   fprintf(ver, "#define FPGA_DATE_HEX   0x%04d%02d%02d\n", now->tm_year+1900, now->tm_mon+1, now->tm_mday);
   fprintf(ver, "#define FPGA_TIME_HEX   0x00%02d%02d%02d\n", now->tm_hour, now->tm_min, now->tm_sec);
   fprintf(ver, "#define FPGA_GIT_REV    \"%s\"\n", git_rev);
   fprintf(ver, "#define FPGA_GIT_AUTH   \"%s\"\n", git_auth);
   fprintf(ver, "#define FPGA_GIT_EMAIL  \"%s\"\n", git_email);
   fprintf(ver, "#define FPGA_GIT_DATE   \"%s\"\n", git_date);

   fclose(ver);
   fflush(ver);

   //
   // Create FPGA VHDL Version File
   //
   // Open File for Writing
   if  ((ver = fopen(argv[3],"wt")) == NULL) {
      printf("Error: Unable to open %s for writing\n", argv[3]);
      exit (-1);
   }
   fprintf(ver, "library ieee;\n");
   fprintf(ver, "use ieee.std_logic_1164.all;\n");
   fprintf(ver, "\n");
   fprintf(ver, "package fpga_ver is\n");
   fprintf(ver, "\n");
   fprintf(ver, "   constant C_BUILD_MAJOR     : std_logic_vector(7 downto 0)   := X\"%02X\";\n", build_major);
   fprintf(ver, "   constant C_BUILD_MINOR     : std_logic_vector(7 downto 0)   := X\"%02X\";\n", build_minor);
   fprintf(ver, "   constant C_BUILD_NUM       : std_logic_vector(7 downto 0)   := X\"%02X\";\n", build_num);
   fprintf(ver, "   constant C_BUILD_INC       : std_logic_vector(7 downto 0)   := X\"%02X\";\n", build_inc);
   fprintf(ver, "   constant C_BUILD_SYSID     : std_logic_vector(31 downto 0)  := X\"%08X\";\n", build_sysid);
   fprintf(ver, "   constant C_BUILD_VER_HEX   : std_logic_vector(31 downto 0)  := X\"%08X\";\n", build_hex);
   fprintf(ver, "   constant C_BUILD_TIME      : string                         := \"%s\";\n", build_time);
   fprintf(ver, "   constant C_BUILD_DATE      : string                         := \"%s\";\n", build_date);
   fprintf(ver, "   constant C_BUILD_USER      : string                         := \"%s\";\n", build_user);
   fprintf(ver, "   constant C_BUILD_STR       : string                         := \"%s\";\n", build_str2);
   fprintf(ver, "   constant C_BUILD_LO        : string                         := \"%s\";\n", build_lo);
   fprintf(ver, "   constant C_BUILD_HI        : string                         := \"%s\";\n", build_hi);
   fprintf(ver, "   constant C_BUILD_STRING    : string                         := \"%s\";\n", build_str1);
   fprintf(ver, "   constant C_BUILD_EPOCH     : integer                        := %d;\n", (uint32_t)clock);
   fprintf(ver, "   constant C_BUILD_EPOCH_HEX : std_logic_vector(31 downto 0)  := X\"%08X\";\n", (uint32_t)clock);
   fprintf(ver, "   constant C_BUILD_DATE_HEX  : std_logic_vector(31 downto 0)  := X\"%04d%02d%02d\";\n", now->tm_year+1900, now->tm_mon+1, now->tm_mday);
   fprintf(ver, "   constant C_BUILD_TIME_HEX  : std_logic_vector(31 downto 0)  := X\"00%02d%02d%02d\";\n", now->tm_hour, now->tm_min, now->tm_sec);
   fprintf(ver, "   constant C_BUILD_GIT_REV   : string                         := \"%s\";\n", git_rev);
   fprintf(ver, "   constant C_BUILD_GIT_AUTH  : string                         := \"%s\";\n", git_auth);
   fprintf(ver, "   constant C_BUILD_GIT_EMAIL : string                         := \"%s\";\n", git_email);
   fprintf(ver, "   constant C_BUILD_GIT_DATE  : string                         := \"%s\";\n", git_date);
   fprintf(ver, "\n");
   fprintf(ver, "end fpga_ver;\n");

   fclose(ver);
   fflush(ver);

   //
   // Open build.h and read BUILD_HI, sanity check
   //
   if  ((ver = fopen(argv[2],"rt")) != NULL) {
      while (fgets(in_line, sizeof(in_line), ver) != NULL) {
         token = strtok(in_line, " ");
         token = strtok(NULL, " ");
         if (token == NULL) continue;
         if (strcmp(token, "BUILD_HI") == 0) {
            token = strtok(NULL, "\n");
            break;
         }
      }
      fclose(ver);
   }

   printf("BUILD_HI : %s\n\n", build_hi);

   return 0;
}
