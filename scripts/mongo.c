#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    FILE *fp;
    fp = fopen("charts/open5gs/templates/mongo-ue-init-script.yaml", "w"); // Open file for writing

    if (fp == NULL) {
        printf("Error opening file!\n");
        return 1;
    }

    // Write YAML content line by line using fprintf
    
    int i;
    int n = atoi(argv[1]);
    
        fprintf(fp, "apiVersion: v1\n");
        fprintf(fp, "kind: ConfigMap\n");
        fprintf(fp, "metadata:\n");
        fprintf(fp, "  name: {{ .Release.Name }}-mongo-ue-init\n");
        fprintf(fp, "  labels:\n");
        fprintf(fp, "    epc-mode: job\n");
        fprintf(fp, "data:\n");
        fprintf(fp, "  ue-init.sh: |\n");
        fprintf(fp, "     wget https://github.com/open5gs/open5gs/raw/v2.5.6/misc/db/open5gs-dbctl\n");
        fprintf(fp, "     chmod +x open5gs-dbctl\n");
        fprintf(fp, "\n");
        for(i = 1; i <= n; i++) {
        fprintf(fp, "     if ./open5gs-dbctl --db_uri=mongodb://{{ .Release.Name }}-mongodb-svc/open5gs showfiltered | grep -w {{ .Values.simulator.ue%d.imsi }}; then\n", i);
        fprintf(fp, "          echo \"UE {{ .Values.simulator.ue%d.imsi }} exists, proceeding to delete\"\n", i);
        fprintf(fp, "          ./open5gs-dbctl --db_uri=mongodb://{{ .Release.Name }}-mongodb-svc/open5gs remove {{ .Values.simulator.ue%d.imsi }}\n", i);
        fprintf(fp, "          ./open5gs-dbctl --db_uri=mongodb://{{ .Release.Name }}-mongodb-svc/open5gs add_ue_with_slice {{ .Values.simulator.ue%d.imsi }} {{ .Values.simulator.ue%d.secKey }} {{ .Values.simulator.ue%d.op }} {{ .Values.dnn }} {{ .Values.simulator.ue%d.sst }} {{ .Values.simulator.ue%d.sd }};\n", i, i, i, i, i);
        fprintf(fp, "     else\n");
        fprintf(fp, "          echo \"UE {{ .Values.simulator.ue%d.imsi }} does not exist in the DB, proceeding to add it\"\n", i);
        fprintf(fp, "          ./open5gs-dbctl --db_uri=mongodb://{{ .Release.Name }}-mongodb-svc/open5gs add_ue_with_slice {{ .Values.simulator.ue%d.imsi }} {{ .Values.simulator.ue%d.secKey }} {{ .Values.simulator.ue%d.op }} {{ .Values.dnn }} {{ .Values.simulator.ue%d.sst }} {{ .Values.simulator.ue%d.sd }};\n", i, i, i, i, i);
        fprintf(fp, "     fi\n\n");
    }

    fclose(fp); // Close the file
    return 0;
}

