directory="$HOME"
for thread_count in 1 5 10 20 
do
    cmd="lab3 "$directory" -thread_count "$thread_count
    echo "$cmd"
    $cmd
done
