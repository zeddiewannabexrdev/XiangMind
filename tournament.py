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


class EngineProcess:
    def __init__(self, name, path, is_zeddie=False, use_gui=False):
        self.name = name
        self.path = path
        self.is_zeddie = is_zeddie
        self.use_gui = use_gui

        args = [path]
        if is_zeddie and not use_gui:
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
        if is_zeddie:
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


def play_single_game(engine_red, engine_black, depth=4, movetime=None, sleep_delay=0.5):
    moves_history = []
    max_half_moves = 160  # 80 nước mỗi bên

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

        # Update GUI if available
        if engine_red.use_gui:
            engine_red.update_gui_board(moves_history)
        elif engine_black.use_gui:
            engine_black.update_gui_board(moves_history)

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


def run_tournament(args):
    zeddie_path = args.engine1
    opp_path = args.engine2

    if not os.path.exists(zeddie_path):
        print(f"[LOI] Khong tim thay engine: {zeddie_path}")
        return
    if not ensure_engine(opp_path):
        print(f"[LOI] Khong tim thay engine: {opp_path}")
        return

    print("==================================================================")
    print("        GIAI DAU CO TUONG TU DONG (TOURNAMENT REFEE)")
    print(f"  Engine 1: Zeddie Engine ({zeddie_path})")
    print(f"  Engine 2: Fairy-Stockfish ({opp_path})")
    print(f"  So van dau: {args.rounds} van")
    print(f"  Che do suy nghi: {'Depth ' + str(args.depth) if not args.movetime else str(args.movetime) + 'ms/nuoc'}")
    print(f"  Hien thi GUI Raylib: {'Bat' if args.gui else 'Tat (Headless chay nhanh)'}")
    print("==================================================================\n")

    wins_zeddie = 0
    draws = 0
    losses_zeddie = 0

    elo_zeddie = float(args.elo1)
    elo_opp = float(args.elo2)
    k_factor = 20.0

    for game_no in range(1, args.rounds + 1):
        zeddie_is_red = (game_no % 2 != 0)

        print(f"\n--------------------------------------------------------------")
        print(f"  VAN DAU {game_no}/{args.rounds}")
        if zeddie_is_red:
            print(f"  DO (Di truoc): [Zeddie Engine] vs DEN: [Fairy-Stockfish]")
        else:
            print(f"  DO (Di truoc): [Fairy-Stockfish] vs DEN: [Zeddie Engine]")
        print(f"--------------------------------------------------------------")

        # Khoi dong hai engine cho van dau
        engine1 = EngineProcess("Zeddie", zeddie_path, is_zeddie=True, use_gui=(args.gui and zeddie_is_red))
        engine2 = EngineProcess("Fairy-SF", opp_path, is_zeddie=False, use_gui=False)

        engine_red = engine1 if zeddie_is_red else engine2
        engine_black = engine2 if zeddie_is_red else engine1

        result, moves = play_single_game(
            engine_red,
            engine_black,
            depth=args.depth,
            movetime=args.movetime,
            sleep_delay=args.delay,
        )

        # Don dep engine sau van dau
        engine1.terminate()
        engine2.terminate()

        # Tinh diem van dau
        if result == "DRAW":
            actual_score = 0.5
            draws += 1
            game_result_str = "HOA"
        elif (result == "WHITE" and zeddie_is_red) or (result == "BLACK" and not zeddie_is_red):
            actual_score = 1.0
            wins_zeddie += 1
            game_result_str = "ZEDDIE THANG (+)"
        else:
            actual_score = 0.0
            losses_zeddie += 1
            game_result_str = "ZEDDIE THUA (-)"

        # Tinh toan Elo
        exp_score = expected_score(elo_zeddie, elo_opp)
        delta = calculate_elo_change(k_factor, actual_score, exp_score)
        elo_zeddie += delta
        elo_opp -= delta

        print(f"\n>>> KET QUA VAN {game_no}: {game_result_str}")
        print(f">>> BANG XEP HANG:")
        print(f"    Zeddie: {wins_zeddie} Thang | {draws} Hoa | {losses_zeddie} Thua (Diem: {wins_zeddie + 0.5 * draws} / {game_no})")
        print(f"    Elo Zeddie moi: {elo_zeddie:.1f} ({'+' if delta >= 0 else ''}{delta:.1f})")
        print(f"    Elo Fairy-SF moi: {elo_opp:.1f} ({'-' if delta >= 0 else '+'}{abs(delta):.1f})")

        time.sleep(1.0)

    print("\n==================================================================")
    print("                 TONG KET GIAI DAU")
    print(f"  Tong so van: {args.rounds}")
    print(f"  Zeddie Engine: {wins_zeddie} Thang, {draws} Hoa, {losses_zeddie} Thua")
    win_rate = ((wins_zeddie + 0.5 * draws) / args.rounds) * 100.0
    print(f"  Ti le diem: {win_rate:.1f}%")
    print(f"  Elo cuoi cung cua Zeddie Engine: {elo_zeddie:.1f}")
    print("==================================================================")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Xiangqi Tournament Manager")
    parser.add_argument("--rounds", type=int, default=10, help="Number of games to play")
    parser.add_argument("--depth", type=int, default=4, help="Search depth for moves")
    parser.add_argument("--movetime", type=int, default=None, help="Move time limit in ms")
    parser.add_argument("--delay", type=float, default=0.1, help="Delay in seconds between moves")
    parser.add_argument("--gui", action="store_true", help="Display Raylib GUI viewer")
    parser.add_argument("--engine1", type=str, default="./xiangqi-zeddieengine.exe", help="Path to engine 1")
    parser.add_argument("--engine2", type=str, default="./fairy-stockfish.exe", help="Path to engine 2")
    parser.add_argument("--elo1", type=float, default=1500.0, help="Initial Elo of engine 1")
    parser.add_argument("--elo2", type=float, default=1800.0, help="Initial Elo of engine 2")
    args = parser.parse_args()

    run_tournament(args)
