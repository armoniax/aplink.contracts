initData() {
  updateContract='cleos push action ${otcAccountName} setmerchant "['${merchant}', '${merchant}', '${merchant} detail', '${merchant}@ama.com', '诚信商家，绝对安全']"' -p ${merchant}@active"
  echo "-------initData--------------${merchant}"
  ssh sh-misc "${remoteDockerScrip} '${updateContract}'"
}


remoteDockerScrip='docker exec -i mgp-devnet /bin/bash -c'
otcAccountName=${1}.o

array=("joss1" 
        "chenjunqiang"
        "amaxhu3t3tjd" 
        "jason1")
for i in "${array[@]}"; do   # The quotes are necessary here
    merchant="$i"
    initData
done





