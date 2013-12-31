
IOCP Echo Server for educational purposes only.



# Student Homework

zero byte pre recv로 바꾸는거 (pre-recv lock으로 인한 cpu 사용률 낮추기: io 종류에 따라 overapped context 분리)
AcceptEx로 바꾸는거 (접속 처리 성능 향상)

graceful server shutdown (+command line tool)
half-open connection 감지기능 추가
스마트포인터 사용 (overlapped context 등)

성능 테스트 