initData() {
    cleos push action ${otcAccountName} setmerchant  "[\"${merchant}\",\"${merchant}\",\"${merchant} detail\", \"xxx@ama.com\", \"dddddddd\"]" -p ${merchant}@active
}
otcAccountName=${1}.o

cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog

array=("joss1"
        "chenjunqiang"
        "amaxhu3t3tjd"
        "jason1")
for i in "${array[@]}"; do   # The quotes are necessary here
    merchant="$i"
    initData
done