
merchant=$1
token="eosio.token"
contract=${2}.o
confContract=${2}.0


cleos wallet unlock --password PW5KQzzoYJcijs2wtMpF5Vqk4v8n9FNcxxHj1aqqcjpGJDEkdBrog
merchant=joss1
token="eosio.token"
contract=deotck.o
order_id=1
user=chenjunqiang

#10、商户充值押金
cleos push action ${token} transfer '['${merchant}', "'${contract}'", "1000.000000 CNYD", "deposit"]' -p ${merchant}

#open order
cleos push action ${contract} openorder '['${merchant}', "buy", ["wechat","alipay"],"10.000000 CNYDARC", "1.1000 CNY", "1.000000 CNYDARC", "3.000000 CNYDARC","quick conform 1.1"]' -p ${merchant}

cleos push action ${contract} openorder '['${merchant}', "sell",  ["wechat"],"20.000000 CNYDARC", "1.0000 CNY", "0.500000 CNYDARC", "3.000000 CNYDARC", "quick conform"]' -p ${merchant}@active

#用户吃单
cleos push action ${contract} opendeal '['${user}', "buy",'${order_id}', "1.000000 CNYDARC", 1023, "pay_type:wechat","user_ss","merchant_ss","session_ms"]' -p ${user}@active
cleos push action ${contract} opendeal '['${user}', "sell",'${order_id}', "0.500000 CNYDARC", 2, "pay_type:wechat","user_ss","merchant_ss"]' -p ${user}@active