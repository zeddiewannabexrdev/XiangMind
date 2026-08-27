import sys
if sys.stdout.encoding != 'utf-8':
    sys.stdout.reconfigure(encoding='utf-8')
import subprocess
import time

def start_engine(path, use_gui=False):
    # Khởi động engine. Nếu use_gui=True thì mở cửa sổ, ngược lại chạy ngầm.
    args = [path] if use_gui else [path, "--uci"]
    process = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, encoding='utf-8')
    process.stdin.write("ucci\n")
    process.stdin.flush()
    # Chờ engine sẵn sàng
    while True:
        line = process.stdout.readline()
        if not line: break
        line = line.strip()
        if "ucciok" in line or "uciok" in line:
            break
    return process

def get_bestmove(process, moves_history):
    # Gửi lịch sử ván đấu cho engine
    if not moves_history:
        process.stdin.write("position startpos\n")
    else:
        process.stdin.write("position startpos moves " + " ".join(moves_history) + "\n")
    
    # Ép engine suy nghĩ nhanh (depth 4) để test
    process.stdin.write("go depth 4\n")
    process.stdin.flush()
    
    # Đợi kết quả bestmove
    while True:
        line = process.stdout.readline().strip()
        if line.startswith("bestmove"):
            # Lấy nước đi (ví dụ: bestmove h2e2)
            parts = line.split()
            if len(parts) >= 2:
                return parts[1]
            return None

def main():
    engine_path = r"./build-xq/xiangqi-zeddieengine.exe"
    print("Khoi dong Trong tai giai dau Co Tuong...")
    
    # Mở 1 cửa sổ GUI cho Đỏ, và cho Đen chạy ngầm
    engine_red = start_engine(engine_path, use_gui=True)
    engine_black = start_engine(engine_path, use_gui=False)
    
    moves_history = []
    
    print("Tran dau bat dau: DO (Engine A) vs DEN (Engine B)")
    for half_move in range(1, 101): # Giới hạn 100 nước (50 hiệp)
        is_red_turn = (half_move % 2 != 0)
        current_engine = engine_red if is_red_turn else engine_black
        player_name = "DO " if is_red_turn else "DEN"
        
        move = get_bestmove(current_engine, moves_history)
        if not move or move == "0000": # 0000 thường là mã lỗi hoặc không có nước đi (Bí cờ)
            print(f"[{player_name}] Het nuoc di hoac Bi chieu bi! Tran dau ket thuc.")
            break
            
        moves_history.append(move)
        print(f"Nuoc {half_move} - {player_name} di: {move}")
        
        # Cập nhật bàn cờ cho cửa sổ GUI (engine_red) ngay lập tức
        if not is_red_turn:
            engine_red.stdin.write("position startpos moves " + " ".join(moves_history) + "\n")
            engine_red.stdin.flush()
            
        time.sleep(3.0) # Tăng thời gian nghỉ lên 3 giây để xem chậm rãi hơn
    
    engine_red.terminate()
    engine_black.terminate()
    print("Tong ket van dau (Danh sach nuoc di):")
    print(" ".join(moves_history))

if __name__ == "__main__":
    main()
