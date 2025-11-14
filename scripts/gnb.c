#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Function to create a file and write content to it
void create_file(const char *file_path, const char *content) {
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        perror("Error creating file");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%s", content);
    fclose(file);
}

void get_ip_for_ue(int ue_index, char *ip_buffer, size_t buf_size) {
    int x = 1 + (ue_index / 256);   // subnet increments after every 256 UEs
    int y = ue_index % 256;         // host part

    snprintf(ip_buffer, buf_size, "10.0.%d.%d", x, y);
}

// Function to create a folder with the specified content
void create_folder(int folder_num) {
    char folder_name[200];
    char folder_path[300];

    // Create folder name
    snprintf(folder_name, sizeof(folder_name), "charts/my5GRan-Tester/%d", folder_num);

    // Create the main folder
    mkdir(folder_name, 0777) != 0;

    // Create chart.yaml content
    const char *chart_yaml_content = 
        "apiVersion: v2\n"
        "name: my5GRanTester\n"
        "description: A Helm chart for my5GRanTester\n"
        "type: application\n"
        "version: 0.0.1\n"
        "appVersion: 0.0.1\n";
    snprintf(folder_path, sizeof(folder_path), "%s/Chart.yaml", folder_name);
    create_file(folder_path, chart_yaml_content);

    // Calculate the new IP address
    char controlDataifIP[20];
    get_ip_for_ue(folder_num - 1, controlDataifIP, sizeof(controlDataifIP));

    // printf("UE %d -> IP = %s\n", folder_num, controlDataifIP);

    // Create values.yaml content with updated IP
    char values_yaml_content[5000];
    snprintf(values_yaml_content, sizeof(values_yaml_content), 
        "image:\n"
        "  repository: wriddhiraaj/my5g-ran-tester\n"
        "  pullPolicy: IfNotPresent\n"
        "  tag: \"latest\"\n\n"
        "config:\n"
        "  amfVIP: \"10.0.0.1\"\n"      //do we have to change
        "  amfPort: \"38412\"\n"
        "  controlDataifIP: \"%s\"\n"
        "  controlDataifNetMask: \"20\"\n"   //changed from 24 to 20
        "  msin: \"0000000031\"\n"
        "  mcc: \"208\"\n"
        "  mnc: \"93\"\n"
        "  tac: \"000007\"\n"
        "  opc: \"63bfa50ee6523365ff14c1f45f88737d\"\n"
        "  key: \"0C0A34601D4F07677303652C0462535B\"\n"
        "  dnn: \"internet\"\n"
        "  sst: \"01\"\n"
        "  sd: \"000001\"\n", controlDataifIP);
    snprintf(folder_path, sizeof(folder_path), "%s/values.yaml", folder_name);
    create_file(folder_path, values_yaml_content);

    // Create templates folder
    snprintf(folder_path, sizeof(folder_path), "%s/templates", folder_name);
    if (mkdir(folder_path, 0777) != 0) {
        // perror("Error creating templates folder");
        // exit(EXIT_FAILURE);
    }

    // Create rantester-configmap.yaml content
    const char *rantester_configmap_content = 
        "apiVersion: v1\n"
        "kind: ConfigMap\n"
        "metadata:\n"
        "  name: {{ .Release.Name }}-config\n"
        "  labels:\n"
        "    app: rantester\n"
        "data:\n"
        "  config.yml: |\n"
        "    gnodeb:\n"
        "      controlif:\n"
        "        ip: \"{{ .Values.config.controlDataifIP }}\"\n"
        "        port: 9487\n"
        "      dataif:\n"
        "        ip: \"{{ .Values.config.controlDataifIP }}\"\n"
        "        port: 2152\n"
        "      plmnlist:\n"
        "        mcc: \"{{ .Values.config.mcc }}\"\n"
        "        mnc: \"{{ .Values.config.mnc }}\"\n"
        "        tac: \"{{ .Values.config.tac }}\"\n"
        "        gnbid: \"0000001\"\n"
        "      slicesupportlist:\n"
        "        sst: \"{{ .Values.config.sst }}\"\n"
        "        sd: \"{{ .Values.config.sd }}\"\n"
        "\n"
        "    ue:\n"
        "      msin: \"{{ .Values.config.msin }}\"\n"
        "      key: \"{{ .Values.config.key }}\"\n"
        "      opc: \"{{ .Values.config.opc }}\"\n"
        "      amf: \"8000\"\n"
        "      sqn: \"0000000\"\n"
        "      dnn: \"{{ .Values.config.dnn }}\"\n"
        "      hplmn:\n"
        "        mcc: \"{{ .Values.config.mcc }}\"\n"
        "        mnc: \"{{ .Values.config.mnc }}\"\n"
        "      snssai:\n"
        "        sst: {{ .Values.config.sst }}\n"
        "        sd: \"{{ .Values.config.sd }}\"\n"
        "      integrity:\n"
        "        nia0: false\n"
        "        nia1: false\n"
        "        nia2: true\n"
        "      ciphering:\n"
        "        nea0: true\n"
        "        nea1: false\n"
        "        nea2: false\n"
        "    amfif:\n"
        "      ip: \"{{ .Values.config.amfVIP }}\"\n"
        "      port: {{ .Values.config.amfPort }}\n"
        "\n"
        "    logs:\n"
        "      level: 4\n";
    snprintf(folder_path, sizeof(folder_path), "%s/templates/rantester-configmap.yaml", folder_name);
    create_file(folder_path, rantester_configmap_content);

    // Create rantester-deployment.yaml content
    const char *rantester_deployment_content = 
        "apiVersion: apps/v1\n"
        "kind: Deployment\n"
        "metadata:\n"
        "  name: {{ .Release.Name }}-simulator\n"
        "  labels:\n"
        "    app: ran-tester\n"
        "spec:\n"
        "  replicas: 1\n"
        "  selector:\n"
        "    matchLabels:\n"
        "      app: ran-tester\n"
        "  template:\n"
        "    metadata:\n"
        "      annotations:\n"
        "         k8s.v1.cni.cncf.io/networks: '[\n"
        "                 { \"name\": \"core5g-def\",\n"
        "                   \"ips\": [ {{- cat .Values.config.controlDataifIP \"/\" .Values.config.controlDataifNetMask | nospace | quote }} ] }\n"
        "         ]'\n"
        "      labels:\n"
        "        mode: simulator\n"
        "        app: ran-tester\n"
        "    spec:\n"
        "      containers:\n"
        "      - name: ran\n"
        "        image: \"{{ .Values.image.repository }}:{{ .Values.image.tag }}\"\n"
        "        imagePullPolicy: {{ .Values.image.pullPolicy }}\n"
        "        command: [\"/bin/sh\", \"-c\"]\n"
        "        args:\n"
        "        - ip route add {{ .Values.config.amfVIP }};\n"
        "          sleep infinity;\n"
        "        securityContext:\n"
        "          privileged: true\n"
        "        volumeMounts:\n"
        "        - name: ran-tester-config\n"
        "          mountPath: /root/go/src/my5G-RANTester/config/config.yml\n"
        "          subPath: config.yml\n"
        "      volumes:\n"
        "        - name: ran-tester-config\n"
        "          configMap:\n"
        "            name: {{ .Release.Name }}-config\n";
    snprintf(folder_path, sizeof(folder_path), "%s/templates/rantester-deployment.yaml", folder_name);
    create_file(folder_path, rantester_deployment_content);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_folders>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_folders = atoi(argv[1]);
    for (int i = 1; i <= num_folders; i++) {
        create_folder(i);
    }

    printf("Folders created successfully.\n");
    return EXIT_SUCCESS;
}

