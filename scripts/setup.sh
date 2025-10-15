gcc scripts/addue.c -o scripts/addue && ./scripts/addue $1
# gcc scripts/time.c -o scripts/time && ./scripts/time $1
gcc scripts/mongo.c -o scripts/mongo && ./scripts/mongo $1
gcc scripts/gnb.c -o scripts/gnb && ./scripts/gnb $1
gcc scripts/macvlan.c -o scripts/macvlan && ./scripts/macvlan $1