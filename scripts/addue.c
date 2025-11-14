#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    FILE *file = fopen("charts/open5gs/values.yaml", "w");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }
    int n = atoi(argv[1]);

    fprintf(file, "open5gs:\n");
    fprintf(file, "  image:\n");
    fprintf(file, "    repository: registry.gitlab.com/infinitydon/registry/open5gs-aio\n");
    fprintf(file, "    pullPolicy: IfNotPresent\n");
    fprintf(file, "    tag: v2.5.6\n\n");

    fprintf(file, "webui:\n");
    fprintf(file, "  image:\n");
    fprintf(file, "    repository: registry.gitlab.com/infinitydon/registry/open5gs-webui\n");
    fprintf(file, "    pullPolicy: IfNotPresent\n");
    fprintf(file, "    tag: \"v2.5.6\"\n\n");

    fprintf(file, "ueImport:\n");
    fprintf(file, "  image:\n");
    fprintf(file, "    repository: free5gmano/nextepc-mongodb\n");
    fprintf(file, "    pullPolicy: IfNotPresent\n");
    fprintf(file, "    tag: \"latest\"\n\n");
	fprintf(file, "simulator:");
	int i,j,m=0;
	
    char imsi[20];   // buffer for 15-digit IMSI + null

    for (i = 1; i <= n; i++) 
    {
        // IMSI = 20893 + 10-digit padded value
        sprintf(imsi, "20893%010d", i+30);

        fprintf(file, "\n   ue%d:\n", i);
        fprintf(file, "     imsi: \"%s\"\n", imsi);
        fprintf(file, "     imei: \"356938035643803\"\n");
        fprintf(file, "     imeiSv: \"4370816125816151\"\n");
        fprintf(file, "     op: \"63bfa50ee6523365ff14c1f45f88737d\"\n");
        fprintf(file, "     secKey: \"0C0A34601D4F07677303652C0462535B\"\n");
        fprintf(file, "     sst: \"1\"\n");
        fprintf(file, "     sd: \"1\"");
    }


    fprintf(file, "\n\ndnn: internet\n\n");

    fprintf(file, "k8swait:\n");
    fprintf(file, "  repository: groundnuty/k8s-wait-for\n");
    fprintf(file, "  tag: v1.6\n");
    fprintf(file, "  pullPolicy: IfNotPresent\n\n");

    fprintf(file, "k8s:\n");
    fprintf(file, "  interface: eth0\n\n");

    fprintf(file, "amf1:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.4\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");

    fprintf(file, "amf2:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.5\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");
    
    fprintf(file, "amf3:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.6\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");    

    fprintf(file, "amf4:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.7\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");

    fprintf(file, "amf5:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.8\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");

    fprintf(file, "amf6:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.9\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");

    fprintf(file, "amf7:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.10\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");

    fprintf(file, "amf8:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.11\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");

    fprintf(file, "amf9:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.12\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");

    fprintf(file, "amf10:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.13\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");

    fprintf(file, "amf11:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.14\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");

    fprintf(file, "amf12:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.15\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");

    fprintf(file, "amf13:\n");
    fprintf(file, "  mcc: 208\n");
    fprintf(file, "  mnc: 93\n");
    fprintf(file, "  tac: 7\n");
    fprintf(file, "  networkName: Open5GS\n");
    fprintf(file, "  ngapInt: net1\n");
    fprintf(file, "  multusN2IP: 10.0.0.16\n");
    fprintf(file, "  multusN2NetworkMask: 20\n\n");


    fprintf(file, "smf:\n");
    fprintf(file, "  N4Int: net1\n");
    fprintf(file, "  multusN4IP: 10.0.0.2\n");
    fprintf(file, "  multusN4NetworkMask: 20\n\n");

    fprintf(file, "upf:\n");
    fprintf(file, "  N3N4Int: net1\n");
    fprintf(file, "  multusN3N4IP: 10.0.0.3\n");   
    fprintf(file, "  multusN3N4NetworkMask: 20\n");
    fprintf(file, "  multusN3N4GW: 10.0.0.1\n\n");  // Ig it is lb IP so it will also get changed from 10.0.3.1 -> 10.0.0.1

    fprintf(file, "nssf:\n");
    fprintf(file, "  sst: \"1\"\n");
    fprintf(file, "  sd: \"1\"\n\n");

    fprintf(file, "prometheus:\n");
    fprintf(file, "  nodeExporter:\n");
    fprintf(file, "     repository: quay.io/prometheus/node-exporter\n");
    fprintf(file, "     tag: v1.3.1\n");
    fprintf(file, "     pullPolicy: IfNotPresent\n");

    fclose(file);

    return 0;
}

