#include "labyrinth.h"
#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printMap(Labyrinth *labyrinth) {
  for (int i = 0; i < labyrinth->rows; i++) {
    printf("%.*s\n", labyrinth->cols, labyrinth->map[i]);
  }
}

void printUsage() {
  printf("Usage:\n");
  printf("  labyrinth --map map.txt --player id\n");
  printf("    (Prints the current map state for the specified player)\n");
  printf("  labyrinth --map map.txt --player id --move direction\n");
  printf(
      "    (Moves the player in the specified direction and saves the map)\n");
  printf("  labyrinth --version\n");
  printf("    (Prints version information)\n");
  printf("\nOptions:\n");
  printf("  -m, --map FILE       Path to the map file (required unless "
         "--version)\n");
  printf("  -p, --player ID      Player ID ('0'-'9') (required unless "
         "--version)\n");
  printf("  -d, --move DIR       Direction to move (up, down, left, right)\n");
  printf("  -v, --version        Show version information and exit\n");
  printf("  -h, --help           Show this help message and exit\n");
}

int main(int argc, char *argv[]) {
  char *map_filename = NULL;
  char *player_id_str = NULL;
  char *move_direction = NULL;
  bool version_flag = false;
  bool help_flag = false;

  int c;
  int option_index = 0;

  static struct option long_options[] = {{"map", required_argument, 0, 'm'},
                                         {"player", required_argument, 0, 'p'},
                                         {"move", required_argument, 0, 'd'},
                                         {"version", no_argument, 0, 'v'},
                                         {"help", no_argument, 0, 'h'},
                                         {0, 0, 0, 0}};

  const char *short_options = "m:p:d:vh";

  while ((c = getopt_long(argc, argv, short_options, long_options,
                          &option_index)) != -1) {
    switch (c) {
    case 'm':
      map_filename = optarg;
      break;
    case 'p':
      player_id_str = optarg;
      break;
    case 'd':
      move_direction = optarg;
      break;
    case 'v':
      version_flag = true;
      break;
    case 'h':
      help_flag = true;
      break;
    case '?':
      fprintf(stderr, "Try 'labyrinth --help' for more information.\n");
      return 1;
    default:

      fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
      return 1;
    }
  }

  if (help_flag) {
    printUsage();
    return 0;
  }

  if (version_flag) {
    if (optind < argc) {
      fprintf(stderr, "Error: Unexpected arguments after --version.\n");
      printUsage();
      return 1;
    }

    if (map_filename || player_id_str || move_direction) {
      fprintf(stderr,
              "Error: Cannot combine --version with other game options.\n");
      printUsage();
      return 1;
    }
    printf("%s\n", VERSION_INFO);
    return 0;
  }

  if (!map_filename) {
    fprintf(stderr, "Error: --map option is required.\n");
    printUsage();
    return 1;
  }
  if (!player_id_str) {
    fprintf(stderr, "Error: --player option is required.\n");
    printUsage();
    return 1;
  }

  if (strlen(player_id_str) != 1 || !isValidPlayer(player_id_str[0])) {
    fprintf(
        stderr,
        "Error: Invalid player ID '%s'. Must be a single digit ('0'-'9').\n",
        player_id_str);
    return 1;
  }
  char player_id = player_id_str[0];

  if (move_direction) {
    if (strcmp(move_direction, "up") != 0 &&
        strcmp(move_direction, "down") != 0 &&
        strcmp(move_direction, "left") != 0 &&
        strcmp(move_direction, "right") != 0) {
      fprintf(stderr,
              "Error: Invalid move direction '%s'. Must be one of: up, down, "
              "left, right.\n",
              move_direction);
      return 1;
    }
  }

  if (optind < argc) {
    fprintf(stderr, "Error: Unexpected arguments found: ");
    while (optind < argc) {
      fprintf(stderr, "%s ", argv[optind++]);
    }
    fprintf(stderr, "\n");
    printUsage();
    return 1;
  }

  Labyrinth lab;

  if (!loadMap(&lab, map_filename)) {

    return 1;
  }

  Position player_pos = findPlayer(&lab, player_id);
  if (player_pos.row == -1) {
    fprintf(stderr, "Error: Player '%c' not found on map '%s'.\n", player_id,
            map_filename);
    return 1;
  }

  if (move_direction) {
    if (movePlayer(&lab, player_id, move_direction)) {

      if (!saveMap(&lab, map_filename)) {

        return 1;
      }

      return 0;
    } else {
      fprintf(stderr, "Error: Move failed for player '%c' in direction '%s'.\n",
              player_id, move_direction);
      return 1;
    }
  } else {
    printMap(&lab);
    return 0;
  }

  return 0;
}

bool isValidPlayer(char playerId) { return isdigit(playerId); }

bool loadMap(Labyrinth *labyrinth, const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "Error: Cannot open map file '%s'.\n", filename);
    return false;
  }

  labyrinth->rows = 0;
  labyrinth->cols = 0;
  char line_buffer[MAX_COLS + 2];

  while (labyrinth->rows < MAX_ROWS &&
         fgets(line_buffer, sizeof(line_buffer), file)) {

    line_buffer[strcspn(line_buffer, "\r\n")] = 0;

    int current_cols = strlen(line_buffer);

    if (labyrinth->rows == 0) {
      if (current_cols == 0) {
        fprintf(stderr,
                "Error: Map file '%s' is empty or first line is empty.\n",
                filename);
        fclose(file);
        return false;
      }
      if (current_cols > MAX_COLS) {
        fprintf(stderr, "Error: Map line %d exceeds maximum columns (%d).\n",
                labyrinth->rows + 1, MAX_COLS);
        fclose(file);
        return false;
      }
      labyrinth->cols = current_cols;
    } else {

      if (current_cols != labyrinth->cols) {
        fprintf(stderr,
                "Error: Inconsistent line length in map file '%s'. Line %d has "
                "%d columns, expected %d.\n",
                filename, labyrinth->rows + 1, current_cols, labyrinth->cols);
        fclose(file);
        return false;
      }
    }

    strncpy(labyrinth->map[labyrinth->rows], line_buffer, labyrinth->cols);

    labyrinth->map[labyrinth->rows][labyrinth->cols] = '\0';
    labyrinth->rows++;
  }

  fclose(file);

  if (labyrinth->rows == 0 && labyrinth->cols == 0) {
    fprintf(stderr,
            "Error: No map data loaded from '%s'. File might be empty or "
            "unreadable.\n",
            filename);
    return false;
  }

  return true;
}

Position findPlayer(Labyrinth *labyrinth, char playerId) {
  Position pos = {-1, -1};
  for (int r = 0; r < labyrinth->rows; ++r) {
    for (int c = 0; c < labyrinth->cols; ++c) {
      if (labyrinth->map[r][c] == playerId) {
        pos.row = r;
        pos.col = c;
        return pos;
      }
    }
  }
  return pos;
}

Position findFirstEmptySpace(Labyrinth *labyrinth) {
  Position pos = {-1, -1};
  for (int r = 0; r < labyrinth->rows; ++r) {
    for (int c = 0; c < labyrinth->cols; ++c) {
      if (labyrinth->map[r][c] == '.') {
        pos.row = r;
        pos.col = c;
        return pos;
      }
    }
  }
  return pos;
}

bool isEmptySpace(Labyrinth *labyrinth, int row, int col) {

  if (row < 0 || row >= labyrinth->rows || col < 0 || col >= labyrinth->cols) {
    return false;
  }

  return labyrinth->map[row][col] == '.';
}

bool movePlayer(Labyrinth *labyrinth, char playerId, const char *direction) {
  Position current_pos = findPlayer(labyrinth, playerId);
  if (current_pos.row == -1) {

    fprintf(stderr, "Error (movePlayer): Player '%c' not found.\n", playerId);
    return false;
  }

  int target_row = current_pos.row;
  int target_col = current_pos.col;

  if (strcmp(direction, "up") == 0) {
    target_row--;
  } else if (strcmp(direction, "down") == 0) {
    target_row++;
  } else if (strcmp(direction, "left") == 0) {
    target_col--;
  } else if (strcmp(direction, "right") == 0) {
    target_col++;
  } else {

    fprintf(stderr, "Error (movePlayer): Invalid direction '%s'.\n", direction);
    return false;
  }

  if (isEmptySpace(labyrinth, target_row, target_col)) {

    labyrinth->map[target_row][target_col] = playerId;
    labyrinth->map[current_pos.row][current_pos.col] = '.';
    return true;
  } else {

    return false;
  }
}

bool saveMap(Labyrinth *labyrinth, const char *filename) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    fprintf(stderr, "Error: Cannot open map file '%s' for writing.\n",
            filename);
    return false;
  }

  for (int i = 0; i < labyrinth->rows; i++) {

    if (fwrite(labyrinth->map[i], 1, labyrinth->cols, file) !=
        labyrinth->cols) {
      fprintf(stderr, "Error: Failed to write row %d to map file '%s'.\n", i,
              filename);
      fclose(file);
      return false;
    }
    if (fputc('\n', file) == EOF) {
      fprintf(stderr,
              "Error: Failed to write newline after row %d to map file '%s'.\n",
              i, filename);
      fclose(file);
      return false;
    }
  }

  fclose(file);
  return true;
}

void dfs(Labyrinth *labyrinth, int row, int col,
         bool visited[MAX_ROWS][MAX_COLS]) {
  if (row < 0 || row >= labyrinth->rows || col < 0 || col >= labyrinth->cols ||
      visited[row][col] || labyrinth->map[row][col] != '.') {
    return;
  }

  visited[row][col] = true;

  dfs(labyrinth, row - 1, col, visited);
  dfs(labyrinth, row + 1, col, visited);
  dfs(labyrinth, row, col - 1, visited);
  dfs(labyrinth, row, col + 1, visited);
}

bool isConnected(Labyrinth *labyrinth) {
  bool visited[MAX_ROWS][MAX_COLS];
  memset(visited, 0, sizeof(visited));

  Position start_pos = findFirstEmptySpace(labyrinth);

  if (start_pos.row == -1) {
    return true;
  }

  dfs(labyrinth, start_pos.row, start_pos.col, visited);

  for (int r = 0; r < labyrinth->rows; ++r) {
    for (int c = 0; c < labyrinth->cols; ++c) {

      if (labyrinth->map[r][c] == '.' && !visited[r][c]) {
        return false;
      }
    }
  }

  return true;
}
