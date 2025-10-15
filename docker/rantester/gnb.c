#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

// Function prototypes
void create_folder(const char *path);
void create_file(const char *path, const char *content);

void create_helm_chart(int index) {
    char folder_name[50];
    sprintf(folder_name, "my5GRanTester%d-helm-chart", index);
    create_folder(folder_name);

    char file_path[100];
    char content[10240];

    // Create chart.yaml file
    sprintf(file_path, "%s/Chart.yaml", folder_name);
    sprintf(content,
            "apiVersion: v2\n"
            "name: my5GRanTester%d\n"
            "description: A Helm chart for my5GRanTester%d\n"
            "version: 0.1.0\n"
            "appVersion: 1.16.0\n"
            "type: application\n", index, index);
    create_file(file_path, content);

    // Create values.yaml file
    sprintf(file_path, "%s/values.yaml", folder_name);
    sprintf(content,
            "image:\n"
            "    repository: registry.gitlab.com/infinitydon/registry/my5g-ran-tester\n"
            "    pullPolicy: IfNotPresent\n"
            "    tag: \"f652b3d\"\n"
            "\n"
            "config:\n"
            "  #loxiLBIP: \"10.0.3.1\"\n"
            "  amfVIP: \"10.0.3.1\"\n"
            "  amfPort: \"38412\"\n"
            "  controlDataifIP: \"10.0.3.%d\"\n"
            "  controlDataifNetMask: \"24\"\n"
            "  msin: \"0000000031\"\n"
            "  mcc: \"208\"\n"
            "  mnc: \"93\"\n"
            "  tac: \"000007\"\n"
            "  opc: \"63bfa50ee6523365ff14c1f45f88737d\"\n"
            "  key: \"0C0A34601D4F07677303652C0462535B\"\n"
            "  dnn: \"internet\"\n"
            "  sst: \"01\"\n"
            "  sd: \"000001\"\n", 10 + index - 1);
    create_file(file_path, content);

    // Create templates folder
    char templates_folder[100];
    sprintf(templates_folder, "%s/templates", folder_name);
    create_folder(templates_folder);

    // Create config.yaml file
    sprintf(file_path, "%s/templates/rantester-configmap.yaml", folder_name);
    sprintf(content,
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
            "        ip: 10.0.3.%d\n"
            "        port: 9487\n"
            "      dataif:\n"
            "        ip: 10.0.3.%d\n"
            "        port: 2152\n"
            "      plmnlist:\n"
            "        mcc: \"{{ .Values.config.mcc }}\"\n"
            "        mnc: \"{{ .Values.config.mnc }}\"\n"
            "        tac: \"{{ .Values.config.tac }}\"\n"
            "        gnbid: \"0000002\"\n"
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
            "        level: 4\n", 10 + index - 1, 10 + index - 1);
    create_file(file_path, content);

    // Create deploy.yaml file
    sprintf(file_path, "%s/templates/rantester-deployment.yaml", folder_name);
    sprintf(content,
            "apiVersion: apps/v1 # for versions before 1.9.0 use apps/v1beta2\n"
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
            "          mountPath: /workspace/my5G-RANTester/config/config.yml\n"
            "          subPath: config.yml\n"
            "      volumes:\n"
            "        - name: ran-tester-config\n"
            "          configMap:\n"
            "            name: {{ .Release.Name }}-config\n");
    create_file(file_path, content);
}

void create_folder(const char *path) {
    if (mkdir(path, 0777) != 0) {
        perror("Error creating folder");
        exit(EXIT_FAILURE);
    }
}

void create_file(const char *path, const char *content) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        perror("Error creating file");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%s", content);
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_folders>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_folders = atoi(argv[1]);
    for (int i = 1; i <= num_folders; i++) {
        create_helm_chart(i);
    }

    printf("Folder structures and files created successfully.\n");

    return 0;
}

