initData() {
    cleos wallet import --private-key ${privKey}
    cleos push action ${otcAccountName} setmerchant  "[\"${merchant}\",\"${merchant}\",\"${merchant} detail\", \"xxx@ama.com\", \"dddddddd\"]" -p ${merchant}@active
}
otcAccountName=${1}.o

cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog

array=("5KJyakfLhb1JGcLPqx9sFoWNoeBGvSk9NBTBCY45rHVXXitGqnT:joss1"
        "5JZNkpLZdb7USfCea44wBsqPsm8us2bDKDXCmGjiDJ96o6pHtjt:chenjunqiang"
        "5K5T8764xLVM5eRCAwNhLKzz7Bys4JDNaKrLU5tbRkD2EtLLnTe:amaxhu3t3tjd"
        "5KVsZcENtALWsedrC1k4jnbXuDiNJqQmshgLqpteG5fHJqiQgKA:jason1")
for i in "${array[@]}"; do   # The quotes are necessary here
    IFS=':' read -r -a arr <<< "$i"
    # initData
    privKey=${arr[0]}
    merchant=${arr[1]}
done