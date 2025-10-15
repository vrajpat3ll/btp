cd 5gcore-sctp-loadbalancer

sudo kind create cluster --config config-3node.yml

sudo kubectl create -f multus-daemonset.yml

sudo kubectl create ns open5gs

sudo kubectl create ns loadbalancer

sudo kubectl create ns ran-simulator1

sudo kubectl create ns ran-simulator2

sudo kubectl create ns ran-simulator3

sudo kubectl create ns ran-simulator4

sudo kubectl create ns ran-simulator5

sudo kubectl create ns ran-simulator6

sudo kubectl create ns ran-simulator7

sudo kubectl create ns ran-simulator8

sudo kubectl create ns ran-simulator9

sudo kubectl create ns ran-simulator10

sudo kubectl create ns ran-simulator11

sudo kubectl create ns ran-simulator12

sudo kubectl create ns ran-simulator13

sudo kubectl create ns ran-simulator14

sudo kubectl create ns ran-simulator15

sudo kubectl create ns ran-simulator16

sudo kubectl create ns ran-simulator17

sudo kubectl create ns ran-simulator18

sudo kubectl create ns ran-simulator19

sudo kubectl create ns ran-simulator20

sudo kubectl create ns ran-simulator21

sudo kubectl create ns ran-simulator22

sudo kubectl create ns ran-simulator23

sudo kubectl create ns ran-simulator24

sudo kubectl create ns ran-simulator25

sudo kubectl create ns ran-simulator26

sudo kubectl create ns ran-simulator27

sudo kubectl create ns ran-simulator28

sudo kubectl create ns ran-simulator29

sudo kubectl create ns ran-simulator30

sudo kubectl create ns ran-simulator31

sudo kubectl create ns ran-simulator32

sudo kubectl create ns ran-simulator33

sudo kubectl create ns ran-simulator34

sudo kubectl create ns ran-simulator35

sudo kubectl create ns ran-simulator36

sudo kubectl create ns ran-simulator37

sudo kubectl create ns ran-simulator38

sudo kubectl create ns ran-simulator39

sudo kubectl create ns ran-simulator40

sudo kubectl create ns ran-simulator41

sudo kubectl create ns ran-simulator42

sudo kubectl create ns ran-simulator43

sudo kubectl create ns ran-simulator44

sudo kubectl create ns ran-simulator45

sudo kubectl create ns ran-simulator46

sudo kubectl create ns ran-simulator47

sudo kubectl create ns ran-simulator48

sudo kubectl create ns ran-simulator49

sudo kubectl create ns ran-simulator50

sudo kubectl create ns ran-simulator51

sudo kubectl create ns ran-simulator52

sudo kubectl create ns ran-simulator53

sudo kubectl create ns ran-simulator54

sudo kubectl create ns ran-simulator55

sudo kubectl create ns ran-simulator56

sudo kubectl create ns ran-simulator57

sudo kubectl create ns ran-simulator58

sudo kubectl create ns ran-simulator59

sudo kubectl create ns ran-simulator60

sudo kubectl create ns ran-simulator61

sudo kubectl create ns ran-simulator62

sudo kubectl create ns ran-simulator63

sudo kubectl create ns ran-simulator64

sudo kubectl create ns ran-simulator65

sudo kubectl create ns ran-simulator66

sudo kubectl create ns ran-simulator67

sudo kubectl create ns ran-simulator68

sudo kubectl create ns ran-simulator69

sudo kubectl create ns ran-simulator70

sudo kubectl create ns ran-simulator71

sudo kubectl create ns ran-simulator72

sudo kubectl create ns ran-simulator73

sudo kubectl create ns ran-simulator74

sudo kubectl create ns ran-simulator75

sudo kubectl create ns ran-simulator76

sudo kubectl create ns ran-simulator77

sudo kubectl create ns ran-simulator78

sudo kubectl create ns ran-simulator79

sudo kubectl create ns ran-simulator80

sudo kubectl create ns ran-simulator81

sudo kubectl create ns ran-simulator82

sudo kubectl create ns ran-simulator83

sudo kubectl create ns ran-simulator84

sudo kubectl create ns ran-simulator85

sudo kubectl create ns ran-simulator86

sudo kubectl create ns ran-simulator87

sudo kubectl create ns ran-simulator88

sudo kubectl create ns ran-simulator89

sudo kubectl create ns ran-simulator90

sudo curl -LO https://github.com/redhat-nfvpe/koko/releases/download/v0.82/koko_0.82_linux_amd64

sudo chmod +x koko_0.82_linux_amd64

sudo ./koko_0.82_linux_amd64 -d kind-worker,eth1 -d kind-worker2,eth1q

sudo modprobe sctp

sudo kubectl create -f cni-install.yml

sudo kubectl create -f core-5g-macvlan.yml

sudo helm -n open5gs upgrade --install core5g open5gs-helm-charts/

sudo kubectl -n open5gs get po

sudo kubectl apply -f service-account.yaml

sudo kubectl apply -f cluster-role.yaml

sudo kubectl apply -f cluster-role-binding.yaml

sudo helm -n loadbalancer upgrade --install lb Loadbalancer-helm-chart/

sudo kubectl -n loadbalancer get po

sudo helm -n ran-simulator1 upgrade --install sim5g my5GRanTester1-helm-chart/

sudo helm -n ran-simulator2 upgrade --install sim5g my5GRanTester2-helm-chart/

sudo helm -n ran-simulator3 upgrade --install sim5g my5GRanTester3-helm-chart/

sudo helm -n ran-simulator4 upgrade --install sim5g my5GRanTester4-helm-chart/

sudo helm -n ran-simulator5 upgrade --install sim5g my5GRanTester5-helm-chart/

sudo helm -n ran-simulator6 upgrade --install sim5g my5GRanTester6-helm-chart/

sudo helm -n ran-simulator7 upgrade --install sim5g my5GRanTester7-helm-chart/

sudo helm -n ran-simulator8 upgrade --install sim5g my5GRanTester8-helm-chart/

sudo helm -n ran-simulator9 upgrade --install sim5g my5GRanTester9-helm-chart/

sudo helm -n ran-simulator10 upgrade --install sim5g my5GRanTester10-helm-chart/

sudo helm -n ran-simulator11 upgrade --install sim5g my5GRanTester11-helm-chart/

sudo helm -n ran-simulator12 upgrade --install sim5g my5GRanTester12-helm-chart/

sudo helm -n ran-simulator13 upgrade --install sim5g my5GRanTester13-helm-chart/

sudo helm -n ran-simulator14 upgrade --install sim5g my5GRanTester14-helm-chart/

sudo helm -n ran-simulator15 upgrade --install sim5g my5GRanTester15-helm-chart/

sudo helm -n ran-simulator16 upgrade --install sim5g my5GRanTester16-helm-chart/

sudo helm -n ran-simulator17 upgrade --install sim5g my5GRanTester17-helm-chart/

sudo helm -n ran-simulator18 upgrade --install sim5g my5GRanTester18-helm-chart/

sudo helm -n ran-simulator19 upgrade --install sim5g my5GRanTester19-helm-chart/

sudo helm -n ran-simulator20 upgrade --install sim5g my5GRanTester20-helm-chart/

sudo helm -n ran-simulator21 upgrade --install sim5g my5GRanTester21-helm-chart/

sudo helm -n ran-simulator22 upgrade --install sim5g my5GRanTester22-helm-chart/

sudo helm -n ran-simulator23 upgrade --install sim5g my5GRanTester23-helm-chart/

sudo helm -n ran-simulator24 upgrade --install sim5g my5GRanTester24-helm-chart/

sudo helm -n ran-simulator25 upgrade --install sim5g my5GRanTester25-helm-chart/

sudo helm -n ran-simulator26 upgrade --install sim5g my5GRanTester26-helm-chart/

sudo helm -n ran-simulator27 upgrade --install sim5g my5GRanTester27-helm-chart/

sudo helm -n ran-simulator28 upgrade --install sim5g my5GRanTester28-helm-chart/

sudo helm -n ran-simulator29 upgrade --install sim5g my5GRanTester29-helm-chart/

sudo helm -n ran-simulator30 upgrade --install sim5g my5GRanTester30-helm-chart/

sudo helm -n ran-simulator31 upgrade --install sim5g my5GRanTester31-helm-chart/

sudo helm -n ran-simulator32 upgrade --install sim5g my5GRanTester32-helm-chart/

sudo helm -n ran-simulator33 upgrade --install sim5g my5GRanTester33-helm-chart/

sudo helm -n ran-simulator34 upgrade --install sim5g my5GRanTester34-helm-chart/

sudo helm -n ran-simulator35 upgrade --install sim5g my5GRanTester35-helm-chart/

sudo helm -n ran-simulator36 upgrade --install sim5g my5GRanTester36-helm-chart/

sudo helm -n ran-simulator37 upgrade --install sim5g my5GRanTester37-helm-chart/

sudo helm -n ran-simulator38 upgrade --install sim5g my5GRanTester38-helm-chart/

sudo helm -n ran-simulator39 upgrade --install sim5g my5GRanTester39-helm-chart/

sudo helm -n ran-simulator40 upgrade --install sim5g my5GRanTester40-helm-chart/

sudo helm -n ran-simulator41 upgrade --install sim5g my5GRanTester41-helm-chart/

sudo helm -n ran-simulator42 upgrade --install sim5g my5GRanTester42-helm-chart/

sudo helm -n ran-simulator43 upgrade --install sim5g my5GRanTester43-helm-chart/

sudo helm -n ran-simulator44 upgrade --install sim5g my5GRanTester44-helm-chart/

sudo helm -n ran-simulator45 upgrade --install sim5g my5GRanTester45-helm-chart/

sudo helm -n ran-simulator46 upgrade --install sim5g my5GRanTester46-helm-chart/

sudo helm -n ran-simulator47 upgrade --install sim5g my5GRanTester47-helm-chart/

sudo helm -n ran-simulator48 upgrade --install sim5g my5GRanTester48-helm-chart/

sudo helm -n ran-simulator49 upgrade --install sim5g my5GRanTester49-helm-chart/

sudo helm -n ran-simulator50 upgrade --install sim5g my5GRanTester50-helm-chart/

sudo helm -n ran-simulator51 upgrade --install sim5g my5GRanTester51-helm-chart/

sudo helm -n ran-simulator52 upgrade --install sim5g my5GRanTester52-helm-chart/

sudo helm -n ran-simulator53 upgrade --install sim5g my5GRanTester53-helm-chart/

sudo helm -n ran-simulator54 upgrade --install sim5g my5GRanTester54-helm-chart/

sudo helm -n ran-simulator55 upgrade --install sim5g my5GRanTester55-helm-chart/

sudo helm -n ran-simulator56 upgrade --install sim5g my5GRanTester56-helm-chart/

sudo helm -n ran-simulator57 upgrade --install sim5g my5GRanTester57-helm-chart/

sudo helm -n ran-simulator58 upgrade --install sim5g my5GRanTester58-helm-chart/

sudo helm -n ran-simulator59 upgrade --install sim5g my5GRanTester59-helm-chart/

sudo helm -n ran-simulator60 upgrade --install sim5g my5GRanTester60-helm-chart/

sudo helm -n ran-simulator61 upgrade --install sim5g my5GRanTester61-helm-chart/

sudo helm -n ran-simulator62 upgrade --install sim5g my5GRanTester62-helm-chart/

sudo helm -n ran-simulator63 upgrade --install sim5g my5GRanTester63-helm-chart/

sudo helm -n ran-simulator64 upgrade --install sim5g my5GRanTester64-helm-chart/

sudo helm -n ran-simulator65 upgrade --install sim5g my5GRanTester65-helm-chart/

sudo helm -n ran-simulator66 upgrade --install sim5g my5GRanTester66-helm-chart/

sudo helm -n ran-simulator67 upgrade --install sim5g my5GRanTester67-helm-chart/

sudo helm -n ran-simulator68 upgrade --install sim5g my5GRanTester68-helm-chart/

sudo helm -n ran-simulator69 upgrade --install sim5g my5GRanTester69-helm-chart/

sudo helm -n ran-simulator70 upgrade --install sim5g my5GRanTester70-helm-chart/

sudo helm -n ran-simulator71 upgrade --install sim5g my5GRanTester71-helm-chart/

sudo helm -n ran-simulator72 upgrade --install sim5g my5GRanTester72-helm-chart/

sudo helm -n ran-simulator73 upgrade --install sim5g my5GRanTester73-helm-chart/

sudo helm -n ran-simulator74 upgrade --install sim5g my5GRanTester74-helm-chart/

sudo helm -n ran-simulator75 upgrade --install sim5g my5GRanTester75-helm-chart/

sudo helm -n ran-simulator76 upgrade --install sim5g my5GRanTester76-helm-chart/

sudo helm -n ran-simulator77 upgrade --install sim5g my5GRanTester77-helm-chart/

sudo helm -n ran-simulator78 upgrade --install sim5g my5GRanTester78-helm-chart/

sudo helm -n ran-simulator79 upgrade --install sim5g my5GRanTester79-helm-chart/

sudo helm -n ran-simulator80 upgrade --install sim5g my5GRanTester80-helm-chart/

sudo helm -n ran-simulator81 upgrade --install sim5g my5GRanTester81-helm-chart/

sudo helm -n ran-simulator82 upgrade --install sim5g my5GRanTester82-helm-chart/

sudo helm -n ran-simulator83 upgrade --install sim5g my5GRanTester83-helm-chart/

sudo helm -n ran-simulator84 upgrade --install sim5g my5GRanTester84-helm-chart/

sudo helm -n ran-simulator85 upgrade --install sim5g my5GRanTester85-helm-chart/

sudo helm -n ran-simulator86 upgrade --install sim5g my5GRanTester86-helm-chart/

sudo helm -n ran-simulator87 upgrade --install sim5g my5GRanTester87-helm-chart/

sudo helm -n ran-simulator88 upgrade --install sim5g my5GRanTester88-helm-chart/

sudo helm -n ran-simulator89 upgrade --install sim5g my5GRanTester89-helm-chart/

sudo helm -n ran-simulator90 upgrade --install sim5g my5GRanTester90-helm-chart/

../monitor_pods.sh
