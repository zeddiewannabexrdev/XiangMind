import argparse
import json
import os
import subprocess
import sys
import threading
import time
import requests

PYCHESS_BASE = "https://www.pychess.org"

ENGINE_PATH = os.path.join(os.path.dirname(__file__), "XiangMind.exe")
if not os.path.exists(ENGINE_PATH):
    ENGINE_PATH = os.path.join(os.path.dirname(__file__), "build-xq", "XiangMind.exe")
if not os.path.exists(ENGINE_PATH):
    ENGINE_PATH = os.path.join(os.path.dirname(__file__), "xiangqi-zeddieengine.exe")
if not os.path.exists(ENGINE_PATH):
    ENGINE_PATH = os.path.join(os.path.dirname(__file__), "build-xq", "xiangqi-zeddieengine.exe")

CAPABILITIES = json.dumps({"version": 1, "variants": ["xiangqi"]})


def load_token(cli_token=None):
    if cli_token:
        return cli_token
    env_token = os.environ.get("PYCHESS_TOKEN")
    if env_token:
        return env_token
    config_file = os.path.join(os.path.dirname(__file__), "config.json")
    if os.path.exists(config_file):
        try:
            with open(config_file, "r", encoding="utf-8") as f:
                return json.load(f).get("token")
        except Exception:
            pass
    return None


def get_headers(token):
    return {
        "Authorization": f"Bearer {token}",
        "X-PyChess-Bot-Capabilities": CAPABILITIES,
    }


class EngineWrapper:
    def __init__(self, path):
        self.lock = threading.Lock()
        self.proc = subprocess.Popen(
            [path, "--uci"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
        )
        self._send("uci")
        self._send("setoption name UCI_XiangqiCoordinates value standard")
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

    def get_bestmove(self, moves_str: str, wtime: int, btime: int, winc: int, binc: int) -> str:
        with self.lock:
            if moves_str.strip():
                self._send(f"position startpos moves {moves_str.strip()}")
            else:
                self._send("position startpos")

            cmd = f"go wtime {wtime} btime {btime} winc {winc} binc {binc}"
            self._send(cmd)

            bestmove = None
            while True:
                line = self.proc.stdout.readline()
                if not line:
                    break
                line = line.strip()
                if line.startswith("bestmove"):
                    parts = line.split()
                    if len(parts) >= 2:
                        bestmove = parts[1]
                    break
            return bestmove

    def close(self):
        try:
            self._send("quit")
            self.proc.terminate()
        except Exception:
            pass


def get_account_info(headers):
    try:
        r = requests.get(f"{PYCHESS_BASE}/api/account", headers=headers, timeout=10)
        if r.status_code == 200:
            return r.json()
    except Exception as e:
        print(f"[ERROR] Khong the lay thong tin tai khoan: {e}")
    return None


def accept_challenge(challenge_id: str, headers):
    url = f"{PYCHESS_BASE}/api/challenge/accept/{challenge_id}"
    try:
        r = requests.post(url, headers=headers, timeout=10)
        return r.status_code == 200
    except Exception as e:
        print(f"[ERROR] Loi chap nhan thach dau: {e}")
        return False


def decline_challenge(challenge_id: str, headers, reason="generic"):
    url = f"{PYCHESS_BASE}/api/challenge/decline/{challenge_id}"
    try:
        requests.post(url, headers=headers, json={"reason": reason}, timeout=10)
    except Exception:
        pass


def create_challenge(username: str, headers, rated=True, limit_seconds=180, increment_seconds=2):
    url = f"{PYCHESS_BASE}/api/challenge/{username}"
    payload = {
        "rated": str(rated).lower(),
        "clock.limit": limit_seconds,
        "clock.increment": increment_seconds,
        "variant": "xiangqi",
    }
    try:
        r = requests.post(url, headers=headers, data=payload, timeout=10)
        if r.status_code == 200:
            data = r.json()
            ch_id = data.get("challenge", {}).get("id", "N/A")
            print(f"[THACH DAU] Da gui loi thach dau toi {username} (ID: {ch_id})")
            return True
        else:
            print(f"[LOI] Gui thach dau that bai: HTTP {r.status_code} - {r.text[:120]}")
    except Exception as e:
        print(f"[LOI] Loi khi gui thach dau: {e}")
    return False


def send_move(game_id: str, move: str, headers):
    url = f"{PYCHESS_BASE}/api/bot/game/{game_id}/move/{move}"
    try:
        r = requests.post(url, headers=headers, timeout=10)
        return r.status_code == 200
    except Exception as e:
        print(f"[ERROR] Loi gui nuoc di: {e}")
        return False


def play_game(game_id: str, bot_username: str, engine: EngineWrapper, headers):
    print(f"\n[VAN DAU] Bat dau tran dau: {game_id}")
    stream_url = f"{PYCHESS_BASE}/api/bot/game/stream/{game_id}"

    try:
        resp = requests.get(stream_url, headers=headers, stream=True, timeout=90)
        if resp.status_code != 200:
            print(f"[ERROR] Khong the ket noi vao van dau {game_id}: HTTP {resp.status_code}")
            return

        bot_color = None  # 'white' or 'black'

        for line in resp.iter_lines():
            if not line:
                continue
            event = json.loads(line.decode("utf-8"))
            event_type = event.get("type")

            if event_type == "gameFull":
                white_player = event.get("white", {})
                black_player = event.get("black", {})

                w_name = white_player.get("name") or white_player.get("id")
                b_name = black_player.get("name") or black_player.get("id")

                if w_name and w_name.lower() == bot_username.lower():
                    bot_color = "white"
                else:
                    bot_color = "black"

                print(f"[TRAN DAU] Do: {w_name} vs Den: {b_name} | Bot cam quan: {bot_color.upper()}")

                state = event.get("state", {})
                handle_state(game_id, state, bot_color, engine, headers)

            elif event_type == "gameState":
                handle_state(game_id, event, bot_color, engine, headers)

            elif event_type == "chatLine":
                print(f"[{event.get('username')}]: {event.get('text')}")

    except Exception as e:
        print(f"[ERROR] Loi trong van dau {game_id}: {e}")
    finally:
        print(f"[VAN DAU] Ket thuc tran dau: {game_id}\n")


def handle_state(game_id: str, state: dict, bot_color: str, engine: EngineWrapper, headers):
    status = state.get("status")
    if status and status != "started":
        print(f"[VAN DAU] Trang thai ket thuc: {status}, nguoi thang: {state.get('winner')}")
        return

    moves_str = state.get("moves", "").strip()
    moves_list = moves_str.split() if moves_str else []

    turn = "white" if len(moves_list) % 2 == 0 else "black"

    if turn == bot_color:
        wtime = state.get("wtime", 60000)
        btime = state.get("btime", 60000)
        winc = state.get("winc", 1000)
        binc = state.get("binc", 1000)

        my_time = wtime if bot_color == "white" else btime
        print(f"[BOT] Den luot di (Nuoc {len(moves_list) + 1}) | Con lai: {my_time / 1000:.1f}s")

        bestmove = engine.get_bestmove(moves_str, wtime, btime, winc, binc)
        if bestmove:
            print(f"[BOT] Nuoc di chon: {bestmove}")
            success = send_move(game_id, bestmove, headers)
            if not success:
                print(f"[CANH BAO] Gui nuoc di {bestmove} that bai!")
        else:
            print("[CANH BAO] Engine khong tra ve nuoc di hop le!")


def start_bot(token, target_challenge=None):
    headers = get_headers(token)
    account = get_account_info(headers)
    if not account:
        print("[LOI] Khong the xac thuc token voi PyChess! Kiem tra lai token trong config.json hoac --token.")
        return

    bot_username = account.get("username")
    title = account.get("title", "")
    print(f"==================================================")
    print(f"   PYCHESS XIANGQI BOT RUNNER")
    print(f"   Tai khoan: [{title}] {bot_username}")
    print(f"   Engine: {ENGINE_PATH}")
    print(f"   Che do: Tu dong nhan thach dau Co Tuong (Xiangqi)")
    print(f"==================================================\n")

    print("[KHOI TAO] Dang khoi dong XiangMind Engine...")
    engine = EngineWrapper(ENGINE_PATH)
    print("[KHOI TAO] Engine da san sang!\n")

    if target_challenge:
        print(f"[THACH DAU] Dang gui thach dau toi: {target_challenge}...")
        create_challenge(target_challenge, headers, rated=True, limit_seconds=180, increment_seconds=2)

    event_url = f"{PYCHESS_BASE}/api/stream/event"
    print(f"[KET NOI] Dang lang nghe su kien tu PyChess...")

    while True:
        try:
            resp = requests.get(event_url, headers=headers, stream=True, timeout=90)
            if resp.status_code != 200:
                print(f"[CANH BAO] Stream su kien tra ve ma {resp.status_code}. Thu lai sau 5s...")
                time.sleep(5)
                continue

            for line in resp.iter_lines():
                if not line:
                    continue
                try:
                    event = json.loads(line.decode("utf-8"))
                except Exception:
                    continue

                event_type = event.get("type")

                if event_type == "challenge":
                    ch = event.get("challenge", {})
                    ch_id = ch.get("id")
                    variant = ch.get("variant", {}).get("key")
                    challenger = ch.get("challenger", {}).get("name", "N/A")
                    rated = ch.get("rated", False)

                    if variant == "xiangqi":
                        print(f"\n[THACH DAU] Nhan loi thach dau tu {challenger} | Rated: {rated} -> DANG DONG Y...")
                        if accept_challenge(ch_id, headers):
                            print(f"[THACH DAU] Da dong y thach dau {ch_id}!")
                        else:
                            print(f"[THACH DAU] Dong y that bai {ch_id}")
                    else:
                        print(f"[TU CHOI] Tu choi thach dau bien the khong ho tro: {variant} tu {challenger}")
                        decline_challenge(ch_id, headers, "variant")

                elif event_type == "gameStart":
                    game_info = event.get("game", {})
                    game_id = game_info.get("id")
                    if game_id:
                        game_thread = threading.Thread(
                            target=play_game,
                            args=(game_id, bot_username, engine, headers),
                            daemon=True,
                        )
                        game_thread.start()

        except requests.exceptions.RequestException as e:
            print(f"[MANG] Ket noi bi gian doan ({e}). Dang ket noi lai sau 3 giay...")
            time.sleep(3)
        except Exception as e:
            print(f"[LOI] Loi khong mong muon: {e}")
            time.sleep(3)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="PyChess Xiangqi Bot Runner")
    parser.add_argument("--token", type=str, help="PyChess Bot API Token (optional, can use config.json or PYCHESS_TOKEN)", default=None)
    parser.add_argument("--challenge", type=str, help="Username of opponent to challenge", default=None)
    args = parser.parse_args()

    token = load_token(args.token)
    if not token:
        print("[LOI] Khong tim thay token! Vui long truyen qua --token <TOKEN> hoac tao file config.json.")
        sys.exit(1)

    start_bot(token=token, target_challenge=args.challenge)
