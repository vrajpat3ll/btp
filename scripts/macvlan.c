#include <stdio.h>
#include <stdlib.h>
    
int main(int argc, char *argv[]) {
    FILE *fp;
    fp = fopen("config/core-5g-macvlan.yml", "w");

    if (fp == NULL) {
        printf("Error opening file!\n");
        return 1;
    }
	int n = atoi(argv[1]);
    // Writing YAML content to the file
    fprintf(fp, "apiVersion: \"k8s.cni.cncf.io/v1\"\n");
    fprintf(fp, "kind: NetworkAttachmentDefinition\n");
    fprintf(fp, "metadata:\n");
    fprintf(fp, "  name: core5g-def\n");
    fprintf(fp, "  namespace: open5gs\n");
    fprintf(fp, "spec: \n");
    fprintf(fp, "  config: '{\n");
    fprintf(fp, "            \"cniVersion\": \"0.3.1\",\n");
    fprintf(fp, "            \"plugins\": [\n");
    fprintf(fp, "                {\n");
    fprintf(fp, "                    \"type\": \"macvlan\",\n");
    fprintf(fp, "                    \"capabilities\": { \"ips\": true },\n");
    fprintf(fp, "                    \"master\": \"eth1\",\n");
    fprintf(fp, "                    \"mode\": \"bridge\",\n");
    fprintf(fp, "                    \"ipam\": {\n");
    fprintf(fp, "                        \"type\": \"static\"\n");
    fprintf(fp, "                    }\n");
    fprintf(fp, "                }, {\n");
    fprintf(fp, "                    \"type\": \"tuning\"\n");
    fprintf(fp, "                } ]\n");
    fprintf(fp, "        }'\n");
    
    for (int i = 1; i <= n; i++) {
        fprintf(fp, "---\n");
        fprintf(fp, "apiVersion: \"k8s.cni.cncf.io/v1\"\n");
        fprintf(fp, "kind: NetworkAttachmentDefinition\n");
        fprintf(fp, "metadata:\n");
        fprintf(fp, "  name: core5g-def\n");
        fprintf(fp, "  namespace: ran-simulator%d\n", i);
        fprintf(fp, "spec: \n");
        fprintf(fp, "  config: '{\n");
        fprintf(fp, "            \"cniVersion\": \"0.3.1\",\n");
        fprintf(fp, "            \"plugins\": [\n");
        fprintf(fp, "                {\n");
        fprintf(fp, "                    \"type\": \"macvlan\",\n");
        fprintf(fp, "                    \"capabilities\": { \"ips\": true },\n");
        fprintf(fp, "                    \"master\": \"eth1\",\n");
        fprintf(fp, "                    \"mode\": \"bridge\",\n");
        fprintf(fp, "                    \"ipam\": {\n");
        fprintf(fp, "                        \"type\": \"static\"\n");
        fprintf(fp, "                    }\n");
        fprintf(fp, "                }, {\n");
        fprintf(fp, "                    \"type\": \"tuning\"\n");
        fprintf(fp, "                } ]\n");
        fprintf(fp, "        }'\n");
    }

    
    fprintf(fp, "---\n");
    fprintf(fp, "apiVersion: \"k8s.cni.cncf.io/v1\"\n");
    fprintf(fp, "kind: NetworkAttachmentDefinition\n");
    fprintf(fp, "metadata:\n");
    fprintf(fp, "  name: core5g-def\n");
    fprintf(fp, "  namespace: loadbalancer\n");
    fprintf(fp, "spec:\n");
    fprintf(fp, "  config: '{\n");
    fprintf(fp, "            \"cniVersion\": \"0.3.1\",\n");
    fprintf(fp, "            \"plugins\": [\n");
    fprintf(fp, "                {\n");
    fprintf(fp, "                    \"type\": \"macvlan\",\n");
    fprintf(fp, "                    \"capabilities\": { \"ips\": true },\n");
    fprintf(fp, "                    \"master\": \"eth1\",\n");
    fprintf(fp, "                    \"mode\": \"bridge\",\n");
    fprintf(fp, "                    \"ipam\": {\n");
    fprintf(fp, "                        \"type\": \"static\",\n");
    fprintf(fp, "                        \"addresses\": [\n");
    fprintf(fp, "                          {\n");
    fprintf(fp, "                            \"address\": \"10.0.0.1/20\",\n");  // changes from 10.0.3.1/24 to 10.0.0.1/20
    fprintf(fp, "                            \"gateway\": \"10.0.0.254\"\n");   // changes from 10.0.3.254 to 10.0.0.254
    fprintf(fp, "                          }\n");
    fprintf(fp, "                        ]\n");
    fprintf(fp, "                    }\n");
    fprintf(fp, "                }, {\n");
    fprintf(fp, "                    \"type\": \"tuning\"\n");
    fprintf(fp, "                } ]\n");
    fprintf(fp, "        }'\n");

    fclose(fp);
    return 0;
}

