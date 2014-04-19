directory="~"
for thread_count in 1 5 10 20 
do
    cmd="./lab3 "$directory" -t "$thread_count
    echo "$cmd"
    $cmd
done
