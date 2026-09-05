#include "gui_viewer.h"

#define Color rlColor
#include <raylib.h>
#undef Color
#undef WHITE
#undef BLACK

#include <string>
#include <vector>

void GuiViewer::run(Position& shared_pos, std::mutex& pos_mutex) {
    const int square_size = 80; // Tang kích thu?c ô c? t? 60 lên 80
    const int margin_x = 70;    // Tang margin d? nhu?ng ch? cho ch? s?
    const int margin_y = 70;
    const int panel_width = 450; // Tang chi?u r?ng panel tuong ?ng
    const int screen_width = margin_x * 2 + 8 * square_size + panel_width;
    const int screen_height = margin_y * 2 + 9 * square_size;

    SetTraceLogLevel(7); InitWindow(screen_width, screen_height, "XiangMind - Board Viewer");
    SetTargetFPS(60);

    const char* piece_names[15] = {
        "P", "A", "E", "H", "C", "R", "G",
        "",
        "p", "a", "e", "h", "c", "r", "g"
    };
    
    rlColor rl_black = { 0, 0, 0, 255 };
    rlColor rl_raywhite = { 245, 245, 245, 255 };
    rlColor rl_red = { 230, 41, 55, 255 };
    rlColor rl_darkgray = { 80, 80, 80, 255 };
    rlColor rl_lightgray = { 200, 200, 200, 255 };
    rlColor rl_blue = { 0, 121, 241, 255 };

    while (!WindowShouldClose()) {
        Position pos;
        {
            std::lock_guard<std::mutex> lock(pos_mutex);
            pos = shared_pos;
        }

        BeginDrawing();
        ClearBackground(rl_raywhite);

        // Draw Board Lines
        for (int r = 0; r < 10; ++r) {
            int y = margin_y + (9 - r) * square_size;
            DrawLine(margin_x, y, margin_x + 8 * square_size, y, rl_black);
            
            // Row labels (0-9) d?y xa ra kh?i quân c?
            const char* num_str = TextFormat("%d", r);
            DrawText(num_str, margin_x - 45, y - 10, 24, rl_blue);
        }
        for (int f = 0; f < 9; ++f) {
            int x = margin_x + f * square_size;
            DrawLine(x, margin_y, x, margin_y + 4 * square_size, rl_black);
            DrawLine(x, margin_y + 5 * square_size, x, margin_y + 9 * square_size, rl_black);
            
            // Col labels (a-i) d?y xa ra
            char col_char[2] = { (char)('a' + f), '\0' };
            DrawText(col_char, x - 6, margin_y + 9 * square_size + 30, 24, rl_blue);
            DrawText(col_char, x - 6, margin_y - 45, 24, rl_blue);
        }
        DrawLine(margin_x, margin_y + 4 * square_size, margin_x, margin_y + 5 * square_size, rl_black);
        DrawLine(margin_x + 8 * square_size, margin_y + 4 * square_size, margin_x + 8 * square_size, margin_y + 5 * square_size, rl_black);
        
        // Palaces
        DrawLine(margin_x + 3 * square_size, margin_y + 9 * square_size, margin_x + 5 * square_size, margin_y + 7 * square_size, rl_black);
        DrawLine(margin_x + 5 * square_size, margin_y + 9 * square_size, margin_x + 3 * square_size, margin_y + 7 * square_size, rl_black);
        DrawLine(margin_x + 3 * square_size, margin_y + 0 * square_size, margin_x + 5 * square_size, margin_y + 2 * square_size, rl_black);
        DrawLine(margin_x + 5 * square_size, margin_y + 0 * square_size, margin_x + 3 * square_size, margin_y + 2 * square_size, rl_black);

        // Draw Pieces
        for (int r = 0; r < 10; ++r) {
            for (int f = 0; f < 9; ++f) {
                Square sq = create_square((File)f, (Rank)r);
                Piece pc = pos.piece_on(sq);
                if (pc != NO_PIECE) {
                    int x = margin_x + f * square_size;
                    int y = margin_y + (9 - r) * square_size;
                    
                    rlColor piece_color = color_of(pc) == WHITE ? rl_red : rl_black;
                    
                    // Quân c? cung to hon
                    DrawCircle(x, y, 32, rl_raywhite);
                    DrawCircleLines(x, y, 32, piece_color);
                    DrawCircleLines(x, y, 30, piece_color);
                    
                    const char* txt = piece_names[pc];
                    int text_width = MeasureText(txt, 26);
                    DrawText(txt, x - text_width/2, y - 12, 26, piece_color);
                }
            }
        }

        // Draw Instruction Panel
        int panel_start_x = margin_x * 2 + 8 * square_size;
        DrawRectangle(panel_start_x, 0, panel_width, screen_height, rl_lightgray);
        DrawLine(panel_start_x, 0, panel_start_x, screen_height, rl_darkgray);
        
        int text_x = panel_start_x + 20; // Th?t l? thêm chút
        int text_y = 30; // Kéo text xu?ng chút
        
        DrawText("XIANGMIND - CHEAT SHEET", text_x, text_y, 24, rl_black);
        text_y += 50;
        
        DrawText("[How to Play]", text_x, text_y, 20, rl_red);
        text_y += 30;
        DrawText("Type commands in the black terminal:", text_x, text_y, 18, rl_darkgray);
        text_y += 35;
        
        DrawText("> ucci", text_x, text_y, 20, rl_black);
        text_y += 25;
        DrawText("  Initialize the engine", text_x, text_y, 18, rl_darkgray);
        text_y += 35;
        
        DrawText("> position startpos", text_x, text_y, 20, rl_black);
        text_y += 25;
        DrawText("  Reset board to starting position", text_x, text_y, 18, rl_darkgray);
        text_y += 35;
        
        DrawText("> position startpos moves h2e2", text_x, text_y, 20, rl_black);
        text_y += 25;
        DrawText("  Play a move (e.g. from h2 to e2)", text_x, text_y, 18, rl_darkgray);
        text_y += 25;
        DrawText("  Chain moves: ... moves h2e2 h9g7", text_x, text_y, 18, rl_darkgray);
        text_y += 35;
        
        DrawText("> go", text_x, text_y, 20, rl_black);
        text_y += 25;
        DrawText("  Ask AI to calculate the best move", text_x, text_y, 18, rl_darkgray);
        text_y += 35;
        
        DrawText("> d", text_x, text_y, 20, rl_black);
        text_y += 25;
        DrawText("  Print text-board and FEN string", text_x, text_y, 18, rl_darkgray);
        text_y += 35;
        
        DrawText("> quit", text_x, text_y, 20, rl_black);
        text_y += 25;
        DrawText("  Exit the application", text_x, text_y, 18, rl_darkgray);
        text_y += 50;

        DrawText("[Data Generation for NNUE]", text_x, text_y, 20, rl_red);
        text_y += 30;
        DrawText("Run the executable with args:", text_x, text_y, 18, rl_darkgray);
        text_y += 25;
        DrawText("XiangMind.exe --datagen", text_x, text_y, 18, rl_black);

        EndDrawing();
    }

    CloseWindow();
    std::exit(0);
}
