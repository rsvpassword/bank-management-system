import socket

HOST = '127.0.0.1'
PORT = 8888

def safe_decode(raw: bytes) -> str:
    for enc in ('utf-8', 'gbk', 'gb18030'):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            continue
    return raw.decode('utf-8', errors='replace')

def recv_response(sock):
    lines = []
    buf = b''
    while True:
        data = sock.recv(4096)
        if not data:
            break
        buf += data
        while b'\n' in buf:
            line, buf = buf.split(b'\n', 1)
            line = safe_decode(line).rstrip('\r')
            if line == '###END###':
                return '\n'.join(lines)
            lines.append(line)
    return '\n'.join(lines)

def send_cmd(sock, cmd):
    try:
        data = cmd.encode('gbk')
    except UnicodeEncodeError:
        data = cmd.encode('utf-8')
    sock.sendall(data + b'\n')
    resp = recv_response(sock)
    print(resp)
    print('-' * 50)

def main():
    with socket.create_connection((HOST, PORT)) as s:
        print("已连接服务器，输入命令（QUIT 退出）")
        print("示例：PING / REGADMIN|我的银行|admin|123456 / LOGIN|1|123456")
        print('-' * 50)
        while True:
            try:
                cmd = input('> ').strip()
            except (EOFError, KeyboardInterrupt):
                break
            if not cmd:
                continue
            send_cmd(s, cmd)
            if cmd.upper() == 'QUIT':
                break

if __name__ == '__main__':
    main()
