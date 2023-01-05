# INP Lab 07 - Writeup

## Chal1 & Chal2
第一題有因為兩個 trial 使用同一塊 memory 紀錄 secret, 可以同時進行 trial1 & trial2 來 leak 出 secret 然後往另一邊送，就會噴 flag。

第二題可以從 seed 下手，可以知道某一次執行過後的 seed 後可以知道接下來會出現的 random value，可以透過爆破 pid 來解出 magic。
最後就可以任意構造 secret seed。

## Chal3 
Race condition，用一個連線卡住其中一個 worker，之後朝著塞一個新的往 localhost 做請求的 request。讓 retry 的時候吃到 localhost 的選項。