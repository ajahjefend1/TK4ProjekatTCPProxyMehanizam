set terminal png
set output "Congestion window.png"
set title "Cwnd"
set xlabel "Time"
set ylabel "Time (second)"

set border linewidth 2 
set style line 1 linecolor rgb 'red' linetype 1 linewidth 1 
set grid ytics  
set grid xtics 
plot "prva.dat" title "Congestion wiindow" with linespoints  ls 1 
