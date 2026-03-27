/**
 * @file /cmds/file/ls.c
 *
 * File listing command with support for long listing,
 * sorting, and classification.
 *
 * @created 2006-10-10 - Kenon
 * @last_modified 2026-03-23 - Gesslar
 *
 * @history
 * 2006-10-10 - Kenon - Created
 * 2006-10-10 - Tacitus - Last edited
 * 2026-03-23 - Gesslar - Updated to current conventions
 */

inherit STD_CMD;

#define SIZE_STOP 12
#define DATE_STOP 13
#define NAME_STOP 40
#define SPACES_BETWEEN_FILES 3
#define SPACE_ADDED_BY_FORMAT 2
#define SCREEN_WIDTH 80

#define T_NOMATCH      0
#define T_SHORTOPT     1
#define T_LONGOPT      2
#define T_STRING_VALUE 3
#define T_WHITESP      4

private string arrangeString(string str, int width);
private int numericSort(int field, int sortOrder,
    mixed a, mixed b);
private string filenamePrefix(mixed *fileDetails);
private string filenameSuffix(mixed *fileDetails,
    int classify);

private string currentPath;

void setup() {
  usage_text = "ls [-l] [-a] [-S] [-t] [-r] [-F] [-P] "
    "[file1...]";
  help_text =
"This command lists the contents of a directory "
"(current directory if none is specified). '*' and '?' "
"may be used as wildcards. '*' indicates any number of "
"characters, while '?' indicates a single character.\n"
"\n"
"  -l  Shows extra information about files.\n"
"  -a  Include hidden files.\n"
"  -S  Size sort files.\n"
"  -t  Date sort files.\n"
"  -r  Reverse sort direction.\n"
"  -F  Append an indicator to names.\n"
"  -P  Display path for all dirs.\n"
"\n"
"See also: cd";
}

mixed main(/** @type {STD_PLAYER} */ object caller,
    string arg) {
  mixed *outputFiles;
  mixed *outputFile;
  string outputStr = "";
  int numFiles;
  mixed *tokens;
  int max, i;
  int showAll = 0, sortOrder = 1, sizeSort = 0,
      timeSort = 0, longList = 0, classify = 0,
      alwaysShowPath = 0;
  string *paths = ({});
  int c;

  tokens = reg_assoc(
    arg || "",
    ({
      "-[a-zA-Z]+",
      "--[a-zA-Z]+",
      "[^ \t]+",
      "[ \t]+",
    }),
    ({
      T_SHORTOPT,
      T_LONGOPT,
      T_STRING_VALUE,
      T_WHITESP,
    }),
    T_NOMATCH
  );

  max = sizeof(tokens[0]);
  for(i = 0; i < max; i++) {
    switch(tokens[1][i]) {
      case T_NOMATCH:
      case T_WHITESP:
        break;
      case T_SHORTOPT:
        foreach(c in tokens[0][i][1..<1])
          switch(c) {
            case 'a':
              showAll = 1;
              break;
            case 'r':
              sortOrder = -1;
              break;
            case 'S':
              sizeSort = 1;
              break;
            case 't':
              timeSort = 1;
              break;
            case 'l':
              longList = 1;
              break;
            case 'F':
              classify = 1;
              break;
            case 'P':
              alwaysShowPath = 1;
              break;
            default:
              tell(caller, "\nBad option: "
                + tokens[0][i] + "\n");
              return 1;
          }
        break;
      case T_LONGOPT:
        switch(tokens[0][i]) {
          case "--all":
            showAll = 1;
            break;
          case "--reverse":
            sortOrder = -1;
            break;
          case "--classify":
            classify = 1;
            break;
          default:
            tell(caller, "\nBad option: "
              + tokens[0][i] + "\n");
            return 1;
        }
        break;
      case T_STRING_VALUE:
        paths += ({ tokens[0][i] });
        break;
    }
  }

  if(!sizeof(paths))
    paths = ({ "." });

  foreach(currentPath in paths) {
    currentPath = resolve_path(
      caller->query_env("cwd"), currentPath);

    if(alwaysShowPath || sizeof(paths) > 1)
      outputStr += currentPath + ":\n";

    if(currentPath == "")
      currentPath = caller->query_env("cwd");

    switch(file_size(currentPath)) {
      case -2:
        if(currentPath[<1] != '/')
          currentPath += "/";
        outputFiles = get_dir(currentPath, -1);
        break;
      case -1:
        tell(caller, "\nRead error\n");
        return 1;
      default:
        outputFiles = get_dir(currentPath, -1);
        currentPath = currentPath[0..
          strsrch(currentPath, "/", 1)];
        break;
    }

    if(!outputFiles) {
      tell(caller,
        "Invalid path: " + currentPath + "\n");
      return 1;
    }

    if(!showAll)
      outputFiles = filter(outputFiles,
        (: ($1[0][0] != '.') :));

    numFiles = sizeof(outputFiles);

    if(sizeSort)
      outputFiles = sort_array(outputFiles,
        (: numericSort, 1, sortOrder :));
    else if(timeSort)
      outputFiles = sort_array(outputFiles,
        (: numericSort, 2, sortOrder :));
    else if(sortOrder == -1)
      outputFiles = sort_array(
        outputFiles, sortOrder);

    if(longList) {
      string fileDetailStr = "";
      string temp, thisDir = "";
      int fileSizes = 0;

      foreach(outputFile in outputFiles) {
        if(outputFile[1] == -2) {
          fileDetailStr = "\nd";
        } else {
          fileSizes += outputFile[1];
          fileDetailStr = "\n-";
          temp = sprintf("%d", outputFile[1]);
          fileDetailStr = arrangeString(
            fileDetailStr,
            SIZE_STOP - strlen(temp));
          fileDetailStr += temp;
        }

        fileDetailStr = arrangeString(
          fileDetailStr, DATE_STOP);
        fileDetailStr += ctime(outputFile[2]);

        thisDir += sprintf(
          "%s%s%s{{res}}%s",
          arrangeString(
            fileDetailStr, NAME_STOP),
          filenamePrefix(outputFile),
          outputFile[0],
          filenameSuffix(
            outputFile, classify));
      }

      outputStr += sprintf(
        "%dK (%d bytes) in %d file(s)%s\n",
        fileSizes / 1024, fileSizes,
        numFiles, thisDir);
    } else {
      int largestFileName = 0, screenWidth,
          filesPerLine;

      foreach(outputFile in outputFiles)
        if(largestFileName
            < i = strlen(outputFile[0]))
          largestFileName = i;

      screenWidth = SCREEN_WIDTH;
      if(largestFileName >= screenWidth)
        filesPerLine = 1;
      else
        filesPerLine = (screenWidth - 2)
          / (largestFileName
            + SPACES_BETWEEN_FILES
            + SPACE_ADDED_BY_FORMAT);

      i = 0;
      largestFileName += SPACES_BETWEEN_FILES;

      foreach(outputFile in outputFiles) {
        if(++i == filesPerLine) {
          i = 0;
          outputStr += sprintf(
            "%s%s{{res}}%s\n",
            filenamePrefix(outputFile),
            outputFile[0],
            filenameSuffix(
              outputFile, classify));
        } else {
          outputStr += sprintf("%s%s",
            filenamePrefix(outputFile),
            arrangeString(
              sprintf("%s{{res}}%s",
                outputFile[0],
                filenameSuffix(
                  outputFile, classify)),
              largestFileName));
        }
      }
    }

    outputStr += "\n";
  }

  tell(caller, outputStr);

  return 1;
}

private string arrangeString(string str,
    int width) {
  int i, j, maxi, len;
  string strippedColours;

  if(!width)
    return "";

  strippedColours =
    COLOUR_D->substituteColour(str, "off");
  len = strlen(strippedColours);

  if(len < width)
    return str + repeat_string(" ", width - len);
  if(len == width)
    return str;

  j = 0;
  maxi = sizeof(str);

  for(i = 0; i < maxi; i++) {
    if(str[i] == strippedColours[j]) {
      j++;
      if(j >= width)
        break;
    }
  }

  return str[0..i];
}

private int numericSort(int field, int sortOrder,
    mixed a, mixed b) {
  if(a[field] == b[field])
    return 0;
  if(a[field] > b[field])
    return sortOrder;

  return -sortOrder;
}

private string filenamePrefix(mixed *fileDetails) {
  switch(fileDetails[1]) {
    case -2:
      return "{{0033CC}} ";
    default:
      switch(fileDetails[0][<2..<1]) {
        case ".c":
          if(stat(
              currentPath + fileDetails[0])[2])
            return "{{00FF00}}*";
          return "{{008000}} ";
        case ".h":
          return "{{990000}} ";
        case __SAVE_EXTENSION__:
          return "{{006666}} ";
        default:
          return "{{767676}} ";
      }
  }
}

private string filenameSuffix(mixed *fileDetails,
    int classify) {
  if(classify && fileDetails[1] == -2)
    return "/";

  return " ";
}
