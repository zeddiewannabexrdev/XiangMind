import argparse
import math
import os
import subprocess
import sys
import time

if sys.stdout.encoding != "utf-8":
    sys.stdout.reconfigure(encoding="utf-8")


def calculate_elo_change(k, actual_score, expected_score):
    return k * (actual_score - expected_score)


def expected_score(elo_a, elo_b):
    return 1.0 / (1.0 + math.pow(10.0, (elo_b - elo_a) / 400.0))


def ensure_engine(path):
    if os.path.exists(path):
        return True
    if "fairy-stockfish" in os.path.basename(path).lower():
        print(f"[THONG BAO] Chua co file {path}, dang tu dong tai Fairy-Stockfish tu GitHub...")
        url = "https://github.com/fairy-stockfish/Fairy-Stockfish/releases/download/fairy_sf_14/fairy-stockfish-largeboard_x86-64.exe"
        try:
            import requests

            r = requests.get(url, stream=True, timeout=60)
            if r.status_code == 200:
                with open(path, "wb") as f:
                    for chunk in r.iter_content(chunk_size=65536):
                        if chunk:
                            f.write(chunk)
                print(f"[THANH CONG] Da tai xong {path}!")
                return True
        except Exception as e:
            print(f"[LOI] Khong the tai {path}: {e}")
    return False


class EngineProcess:
    def __init__(self, name, path, is_xiangmind=False, use_gui=False, **kwargs):
        self.name = name
        self.path = path
        self.is_xiangmind = is_xiangmind or kwargs.get("is_zeddie", False)
        self.use_gui = use_gui

        args = [path]
        if self.is_xiangmind:
            if use_gui:
                args.append("--gui")
            else:
                args.append("--uci")

        self.proc = subprocess.Popen(
            args,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            encoding="utf-8",
            bufsize=1,
        )

        # Initialize engine
        if self.is_xiangmind:
            self._send("uci")
            self._send("setoption name UCI_XiangqiCoordinates value standard")
            self._send("isready")
        else:
            self._send("uci")
            self._send("setoption name UCI_Variant value xiangqi")
            self._send("isready")

        self._wait_ready()

    def _send(self, cmd: str):
        self.proc.stdin.write(cmd + "\n")
        self.proc.stdin.flush()

    def _wait_ready(self):
        while True:
            line = self.proc.stdout.readline()
            if not line:
                break
            if "readyok" in line or "uciok" in line:
                break

    def reset_newgame(self):
        self._send("ucinewgame")
        self._send("position startpos")
        self._send("isready")
        self._wait_ready()

    def get_bestmove(self, moves_history, depth=4, movetime=None):
        if not moves_history:
            self._send("position startpos")
        else:
            self._send("position startpos moves " + " ".join(moves_history))

        if movetime:
            self._send(f"go movetime {movetime}")
        else:
            self._send(f"go depth {depth}")

        while True:
            line = self.proc.stdout.readline()
            if not line:
                return None
            line = line.strip()
            if line.startswith("bestmove"):
                parts = line.split()
                if len(parts) >= 2:
                    return parts[1]
                return None

    def update_gui_board(self, moves_history):
        if self.use_gui:
            if moves_history:
                self._send("position startpos moves " + " ".join(moves_history))
            else:
                self._send("position startpos")

    def terminate(self):
        try:
            self._send("quit")
            self.proc.terminate()
        except Exception:
            pass


def play_single_game(engine_red, engine_black, gui_engine=None, depth=4, movetime=None, sleep_delay=0.5):
    moves_history = []
    max_half_moves = 160  # 80 nước mỗi bên

    # Initial empty board update for GUI
    if gui_engine:
        gui_engine.update_gui_board([])

    for half_move in range(1, max_half_moves + 1):
        is_red_turn = (half_move % 2 != 0)
        current_engine = engine_red if is_red_turn else engine_black
        player_name = "DO [" + current_engine.name + "]" if is_red_turn else "DEN [" + current_engine.name + "]"

        move = current_engine.get_bestmove(moves_history, depth=depth, movetime=movetime)

        if not move or move in ("0000", "(none)", "null"):
            winner = "BLACK" if is_red_turn else "WHITE"
            print(f"\n[{player_name}] Het nuoc di / Bi chieu bi! -> {winner} Thang!")
            return winner, moves_history

        moves_history.append(move)
        print(f"Nuoc {half_move:3d} | {player_name}: {move}")

        # Update Raylib GUI window after EVERY move so user sees all moves in real time!
        if gui_engine:
            gui_engine.update_gui_board(moves_history)

        if sleep_delay > 0:
            time.sleep(sleep_delay)

        # Kiem tra lap 3 lan (3-fold repetition)
        if len(moves_history) >= 12:
            last_move = moves_history[-1]
            if moves_history.count(last_move) >= 4:
                print(f"\n[HOA] The co lap lai nhieu lan -> Xu HOA!")
                return "DRAW", moves_history

    print(f"\n[HOA] Vuot qua gioi han {max_half_moves} nuoc -> Xu HOA!")
    return "DRAW", moves_history


def run_tournament(args):
    xiangmind_path = args.engine1
    opp_path = args.engine2

    if not os.path.exists(xiangmind_path):
        print(f"[LOI] Khong tim thay engine: {xiangmind_path}")
        return
    if not ensure_engine(opp_path):
        print(f"[LOI] Khong tim thay engine: {opp_path}")
        return

    show_gui = not args.nogui

    print("==================================================================")
    print("        GIAI DAU CO TUONG TU DONG (TOURNAMENT REFEREE)")
    print(f"  Engine 1: XiangMind ({xiangmind_path})")
    print(f"  Engine 2: Fairy-Stockfish ({opp_path})")
    print(f"  So van dau: {args.rounds} van")
    print(f"  Che do suy nghi: {'Depth ' + str(args.depth) if not args.movetime else str(args.movetime) + 'ms/nuoc'}")
    print(f"  Hien thi GUI Raylib: {'Bat (Xem truc tiep tren ban co)' if show_gui else 'Tat (--nogui chay ngam)'}")
    print("==================================================================\n")

    wins_xiangmind = 0
    draws = 0
    losses_xiangmind = 0

    elo_xiangmind = float(args.elo1)
    elo_opp = float(args.elo2)
    k_factor = 20.0

    print("[KHOI TAO] Dang khoi dong cac engine...")
    engine_xiangmind = EngineProcess("XiangMind", xiangmind_path, is_xiangmind=True, use_gui=show_gui)
    engine_fairy = EngineProcess("Fairy-SF", opp_path, is_xiangmind=False, use_gui=False)
    print("[KHOI TAO] Cac engine da san sang!\n")

    try:
        for game_no in range(1, args.rounds + 1):
            xiangmind_is_red = (game_no % 2 != 0)

            print(f"\n--------------------------------------------------------------")
            print(f"  VAN DAU {game_no}/{args.rounds}")
            if xiangmind_is_red:
                print(f"  DO (Di truoc): [XiangMind] vs DEN: [Fairy-Stockfish]")
            else:
                print(f"  DO (Di truoc): [Fairy-Stockfish] vs DEN: [XiangMind]")
            print(f"--------------------------------------------------------------")

            # Reset ban co cho van moi ma khong can tat mo lai cua so GUI
            engine_xiangmind.reset_newgame()
            engine_fairy.reset_newgame()

            engine_red = engine_xiangmind if xiangmind_is_red else engine_fairy
            engine_black = engine_fairy if xiangmind_is_red else engine_xiangmind

            result, moves = play_single_game(
                engine_red,
                engine_black,
                gui_engine=engine_xiangmind if show_gui else None,
                depth=args.depth,
                movetime=args.movetime,
                sleep_delay=args.delay,
            )

            # Tinh diem van dau
            if result == "DRAW":
                actual_score = 0.5
                draws += 1
                game_result_str = "HOA"
            elif (result == "WHITE" and xiangmind_is_red) or (result == "BLACK" and not xiangmind_is_red):
                actual_score = 1.0
                wins_xiangmind += 1
                game_result_str = "XIANGMIND THANG (+)"
            else:
                actual_score = 0.0
                losses_xiangmind += 1
                game_result_str = "XIANGMIND THUA (-)"

            # Tinh toan Elo
            exp_score = expected_score(elo_xiangmind, elo_opp)
            delta = calculate_elo_change(k_factor, actual_score, exp_score)
            elo_xiangmind += delta
            elo_opp -= delta

            print(f"\n>>> KET QUA VAN {game_no}: {game_result_str}")
            print(f">>> BANG XEP HANG:")
            print(f"    XiangMind: {wins_xiangmind} Thang | {draws} Hoa | {losses_xiangmind} Thua (Diem: {wins_xiangmind + 0.5 * draws} / {game_no})")
            print(f"    Elo XiangMind moi: {elo_xiangmind:.1f} ({'+' if delta >= 0 else ''}{delta:.1f})")
            print(f"    Elo Fairy-SF moi: {elo_opp:.1f} ({'-' if delta >= 0 else '+'}{abs(delta):.1f})")

            time.sleep(1.5)

    except KeyboardInterrupt:
        print("\n[DUNG] Nguoi dung dung giai dau giua chung.")
    finally:
        engine_xiangmind.terminate()
        engine_fairy.terminate()

    print("\n==================================================================")
    print("                 TONG KET GIAI DAU")
    played = wins_xiangmind + draws + losses_xiangmind
    if played > 0:
        print(f"  Tong so van da danh: {played}/{args.rounds}")
        print(f"  XiangMind: {wins_xiangmind} Thang, {draws} Hoa, {losses_xiangmind} Thua")
        win_rate = ((wins_xiangmind + 0.5 * draws) / played) * 100.0
        print(f"  Ti le diem: {win_rate:.1f}%")
        print(f"  Elo cuoi cung cua XiangMind: {elo_xiangmind:.1f}")
    print("==================================================================")


if __name__ == "__main__":
    default_engine1 = "./XiangMind.exe" if os.path.exists("./XiangMind.exe") else ("./build-xq/XiangMind.exe" if os.path.exists("./build-xq/XiangMind.exe") else "./xiangqi-zeddieengine.exe")
    parser = argparse.ArgumentParser(description="Xiangqi Tournament Manager")
    parser.add_argument("--rounds", type=int, default=10, help="Number of games to play (default: 10)")
    parser.add_argument("--depth", type=int, default=4, help="Search depth for moves (default: 4)")
    parser.add_argument("--movetime", type=int, default=None, help="Move time limit in ms")
    parser.add_argument("--delay", type=float, default=0.6, help="Delay in seconds between moves for GUI viewing (default: 0.6s)")
    parser.add_argument("--nogui", action="store_true", help="Disable Raylib GUI window (run headless in terminal only)")
    parser.add_argument("--engine1", type=str, default=default_engine1, help="Path to engine 1 (XiangMind)")
    parser.add_argument("--engine2", type=str, default="./fairy-stockfish.exe", help="Path to engine 2")
    parser.add_argument("--elo1", type=float, default=1500.0, help="Initial Elo of engine 1 (default: 1500)")
    parser.add_argument("--elo2", type=float, default=1800.0, help="Initial Elo of engine 2 (default: 1800)")
    args = parser.parse_args()

    run_tournament(args)
