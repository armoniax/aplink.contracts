
# create account
createAccount() {
    contractAccount=$1
    createAccountScript="cleos wallet create -n ${contractAccount}.wallet --to-console"
    ret=`ssh sh-misc "${remoteDockerScrip} '${createAccountScript}'"`
    echo $ret
    #get priv key 
    privKey=${ret:0-54:53}
    return 0
}

# create contract
# param1: file name
createContract() {
    otcFileName=$1
    otcContractName=$2
 
    unlockScript='cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog'
    ssh sh-misc "${remoteDockerScrip} '${unlockScript}'"

    contractFilePath='/opt/mgp/node_devnet/data/otccontract'
    otcFileName='otcbook'
    setContractScript="cleos set contract ${otcContractName} ${contractFilePath} ${otcFileName}.wasm ${otcFileName}.abi -p ${otcContractName}@active"
    ssh sh-misc "${remoteDockerScrip} '${setContractScript}'"
}

accountTail=$1

otcFileName='otcbook'
#create account
otcContractName="${otcFileName}.${accountTail}"
remoteDockerScrip='docker exec -i mgp-devnet /bin/bash -c'
echo $otcContractName
createAccount $otcContractName
echo "privKey: $privKey"