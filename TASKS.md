# Tasks

> Add tasks here!

- [x] save the logs of our script/app somewhere instead of just on terminal
- [x] rungnb.sh asks for pswd for each ue run; need to change so that it doesn't
- [-] why are we using `wriddhiraj/my5g-ran-tester`? can we use the barebone RANTester?
- [x] TLS handshake timeout in kubectl -> find out why this issue is occuring
  - [x] use `sudo systemctl restart docker` to get around this issue
- [x] refactor codebase
  - [x] GOAL: to be able to have the same ./begin.sh work properly
- [ ] async logging for actual time-evals
- [ ] persistent logging backups
- [ ] use bool to identify the thread and change the conditions in the handle_gnb connections to ensure consistency in count
- [ ] add amf2gnb threads
- [ ] Assignee: _JAIN_, scripts for experimenting and finding results (./exp 100) -> 100 UEs ke saath network create karega, then jo bhi hume results chaiye (metrics like latency for forwarding decision, rtt?, etc.) wo find out karke kahi pe like `data/results` me store karega
- [ ] handle SIGINT(loadbalamcer is closed), detached threads should be killed
- [ ] migratiom latency logging