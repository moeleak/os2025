#include "labyrinth.h"
#include <assert.h>
#include <ctype.h> // For isdigit
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to print the map to stdout
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

  // Define command-line options
  static struct option long_options[] = {{"map", required_argument, 0, 'm'},
                                         {"player", required_argument, 0, 'p'},
                                         {"move", required_argument, 0, 'd'},
                                         {"version", no_argument, 0, 'v'},
                                         {"help", no_argument, 0, 'h'},
                                         {0, 0, 0, 0}};

  // Short options string
  const char *short_options = "m:p:d:vh";

  // Parse command-line arguments
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
    case '?': // getopt_long already printed an error message
      fprintf(stderr, "Try 'labyrinth --help' for more information.\n");
      return 1;
    default:
      // Should not happen with current options
      fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
      return 1;
    }
  }

  // Handle --help
  if (help_flag) {
    printUsage();
    return 0;
  }

  // Handle --version
  if (version_flag) {
    // Allow --version even with other args, but print version and exit 0 if
    // it's the *only* valid arg or first. The tests expect exit 1 if extra args
    // are given with --version.
    if (optind < argc) { // Check if there are non-option arguments left
      fprintf(stderr, "Error: Unexpected arguments after --version.\n");
      printUsage();
      return 1;
    }
    // Check if other options were specified alongside --version (except --help
    // which is handled above)
    if (map_filename || player_id_str || move_direction) {
      fprintf(stderr,
              "Error: Cannot combine --version with other game options.\n");
      printUsage();
      return 1;
    }
    printf("%s\n", VERSION_INFO);
    return 0;
  }

  // Check for required arguments for game operations
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

  // Validate player ID format (single digit)
  if (strlen(player_id_str) != 1 || !isValidPlayer(player_id_str[0])) {
    fprintf(
        stderr,
        "Error: Invalid player ID '%s'. Must be a single digit ('0'-'9').\n",
        player_id_str);
    return 1;
  }
  char player_id = player_id_str[0];

  // Validate move direction if provided
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

  // Check for unexpected non-option arguments
  if (optind < argc) {
    fprintf(stderr, "Error: Unexpected arguments found: ");
    while (optind < argc) {
      fprintf(stderr, "%s ", argv[optind++]);
    }
    fprintf(stderr, "\n");
    printUsage();
    return 1;
  }

  // --- Main Game Logic ---
  Labyrinth lab;

  // Load the map
  if (!loadMap(&lab, map_filename)) {
    // loadMap already prints specific errors
    return 1;
  }

  // Check if the specified player exists on the map
  Position player_pos = findPlayer(&lab, player_id);
  if (player_pos.row == -1) {
    fprintf(stderr, "Error: Player '%c' not found on map '%s'.\n", player_id,
            map_filename);
    return 1;
  }

  // If a move is requested, perform the move and save
  if (move_direction) {
    if (movePlayer(&lab, player_id, move_direction)) {
      // Save the updated map
      if (!saveMap(&lab, map_filename)) {
        // saveMap prints errors
        return 1;
      }
      // Successfully moved and saved, exit 0 (no output needed on success)
      return 0;
    } else {
      // movePlayer failed (e.g., hit a wall, invalid move)
      // movePlayer should ideally print specific errors, but we add a general
      // one
      fprintf(stderr, "Error: Move failed for player '%c' in direction '%s'.\n",
              player_id, move_direction);
      return 1; // Indicate failure
    }
  } else {
    // No move requested, just print the current map state to stdout
    printMap(&lab);
    return 0;
  }

  // Should not be reached, but return 0 for safety
  return 0;
}

// Check if a character is a valid player ID ('0'-'9')
bool isValidPlayer(char playerId) {
  return isdigit(playerId); // Use ctype.h's isdigit for clarity
}

// Load map data from a file into the Labyrinth struct
bool loadMap(Labyrinth *labyrinth, const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "Error: Cannot open map file '%s'.\n", filename);
    return false;
  }

  labyrinth->rows = 0;
  labyrinth->cols = 0;
  char line_buffer[MAX_COLS +
                   2]; // +1 for potential newline, +1 for null terminator

  while (labyrinth->rows < MAX_ROWS &&
         fgets(line_buffer, sizeof(line_buffer), file)) {
    // Remove trailing newline character, if present
    line_buffer[strcspn(line_buffer, "\r\n")] = 0;

    int current_cols = strlen(line_buffer);

    // Determine columns from the first line
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
      // Check if subsequent lines have the same number of columns
      if (current_cols != labyrinth->cols) {
        fprintf(stderr,
                "Error: Inconsistent line length in map file '%s'. Line %d has "
                "%d columns, expected %d.\n",
                filename, labyrinth->rows + 1, current_cols, labyrinth->cols);
        fclose(file);
        return false;
      }
    }

    // Copy the line into the map structure
    strncpy(labyrinth->map[labyrinth->rows], line_buffer, labyrinth->cols);
    // Ensure null termination just in case, though strncpy might not
    // null-terminate if src is longer
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

// Find the position of a given player ID on the map
Position findPlayer(Labyrinth *labyrinth, char playerId) {
  Position pos = {-1, -1}; // Not found initially
  for (int r = 0; r < labyrinth->rows; ++r) {
    for (int c = 0; c < labyrinth->cols; ++c) {
      if (labyrinth->map[r][c] == playerId) {
        pos.row = r;
        pos.col = c;
        return pos; // Found the player
      }
    }
  }
  return pos; // Player not found
}

// Find the position of the first empty space ('.')
Position findFirstEmptySpace(Labyrinth *labyrinth) {
  Position pos = {-1, -1}; // Not found initially
  for (int r = 0; r < labyrinth->rows; ++r) {
    for (int c = 0; c < labyrinth->cols; ++c) {
      if (labyrinth->map[r][c] == '.') {
        pos.row = r;
        pos.col = c;
        return pos; // Found the first empty space
      }
    }
  }
  return pos; // No empty space found
}

// Check if a given coordinate is within map bounds and is an empty space ('.')
bool isEmptySpace(Labyrinth *labyrinth, int row, int col) {
  // Check bounds first
  if (row < 0 || row >= labyrinth->rows || col < 0 || col >= labyrinth->cols) {
    return false;
  }
  // Check if the cell contains an empty space character
  return labyrinth->map[row][col] == '.';
}

// Attempt to move a player in a specified direction
bool movePlayer(Labyrinth *labyrinth, char playerId, const char *direction) {
  Position current_pos = findPlayer(labyrinth, playerId);
  if (current_pos.row == -1) {
    // Should have been checked in main, but double-check
    fprintf(stderr, "Error (movePlayer): Player '%c' not found.\n", playerId);
    return false;
  }

  int target_row = current_pos.row;
  int target_col = current_pos.col;

  // Calculate target position based on direction
  if (strcmp(direction, "up") == 0) {
    target_row--;
  } else if (strcmp(direction, "down") == 0) {
    target_row++;
  } else if (strcmp(direction, "left") == 0) {
    target_col--;
  } else if (strcmp(direction, "right") == 0) {
    target_col++;
  } else {
    // Invalid direction string (should also be caught in main)
    fprintf(stderr, "Error (movePlayer): Invalid direction '%s'.\n", direction);
    return false;
  }

  // Check if the target position is an empty space
  if (isEmptySpace(labyrinth, target_row, target_col)) {
    // Move the player: place player ID in target, place '.' in current position
    labyrinth->map[target_row][target_col] = playerId;
    labyrinth->map[current_pos.row][current_pos.col] = '.';
    return true; // Move successful
  } else {
    // Move failed (hit wall, went out of bounds, or tried to move into another
    // player) No error message here, let main handle the exit status.
    return false; // Move failed
  }
}

// Save the current map state back to a file
bool saveMap(Labyrinth *labyrinth, const char *filename) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    fprintf(stderr, "Error: Cannot open map file '%s' for writing.\n",
            filename);
    return false;
  }

  for (int i = 0; i < labyrinth->rows; i++) {
    // Write the row content followed by a newline
    // Use fwrite for potentially better control/performance with known length
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

// --- Connectivity Check (DFS based) ---

// Recursive Depth-First Search helper function
void dfs(Labyrinth *labyrinth, int row, int col,
         bool visited[MAX_ROWS][MAX_COLS]) {
  // Check bounds and if already visited or not an empty space
  if (row < 0 || row >= labyrinth->rows || col < 0 || col >= labyrinth->cols ||
      visited[row][col] || labyrinth->map[row][col] != '.') {
    return;
  }

  // Mark current cell as visited
  visited[row][col] = true;

  // Recursively visit neighbors (up, down, left, right)
  dfs(labyrinth, row - 1, col, visited); // Up
  dfs(labyrinth, row + 1, col, visited); // Down
  dfs(labyrinth, row, col - 1, visited); // Left
  dfs(labyrinth, row, col + 1, visited); // Right
}

// Check if all empty spaces ('.') in the labyrinth are connected
bool isConnected(Labyrinth *labyrinth) {
  bool visited[MAX_ROWS][MAX_COLS];
  memset(visited, 0, sizeof(visited)); // Initialize visited array to false

  // Find the first empty space to start the DFS from
  Position start_pos = findFirstEmptySpace(labyrinth);

  // If there are no empty spaces, the map is considered connected (vacuously
  // true)
  if (start_pos.row == -1) {
    return true;
  }

  // Perform DFS starting from the first empty space found
  dfs(labyrinth, start_pos.row, start_pos.col, visited);

  // Check if all other empty spaces were visited
  for (int r = 0; r < labyrinth->rows; ++r) {
    for (int c = 0; c < labyrinth->cols; ++c) {
      // If we find an empty space that wasn't visited by DFS, the map is
      // disconnected
      if (labyrinth->map[r][c] == '.' && !visited[r][c]) {
        return false; // Found an unreachable empty space
      }
    }
  }

  // If all empty spaces were visited, the map is connected
  return true;
}

