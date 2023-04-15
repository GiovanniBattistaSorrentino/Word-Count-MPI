mpicc ../word-count.c -o a.out
mkdir out
mpirun --allow-run-as-root -np 1 a.out dir_files out/out1.csv
mpirun --allow-run-as-root -np 2 a.out dir_files out/out2.csv
mpirun --allow-run-as-root -np 3 a.out dir_files out/out3.csv
mpirun --allow-run-as-root -np 4 a.out dir_files out/out4.csv
mpirun --allow-run-as-root -np 5 a.out dir_files out/out5.csv
mpirun --allow-run-as-root -np 6 a.out dir_files out/out6.csv
mpirun --allow-run-as-root -np 7 a.out dir_files out/out7.csv
mpirun --allow-run-as-root -np 8 a.out dir_files out/out8.csv
mpirun --allow-run-as-root -np 9 a.out dir_files out/out9.csv
mpirun --allow-run-as-root -np 10 a.out dir_files out/out10.csv
mpirun --allow-run-as-root -np 11 a.out dir_files out/out11.csv
mpirun --allow-run-as-root -np 12 a.out dir_files out/out12.csv
mpirun --allow-run-as-root -np 13 a.out dir_files out/out13.csv
mpirun --allow-run-as-root -np 14 a.out dir_files out/out14.csv
mpirun --allow-run-as-root -np 15 a.out dir_files out/out15.csv
diff oracolo.csv out/out1.csv
diff oracolo.csv out/out2.csv
diff oracolo.csv out/out3.csv
diff oracolo.csv out/out4.csv
diff oracolo.csv out/out5.csv
diff oracolo.csv out/out6.csv
diff oracolo.csv out/out7.csv
diff oracolo.csv out/out8.csv
diff oracolo.csv out/out9.csv
diff oracolo.csv out/out10.csv
diff oracolo.csv out/out11.csv
diff oracolo.csv out/out12.csv
diff oracolo.csv out/out13.csv
diff oracolo.csv out/out14.csv
diff oracolo.csv out/out15.csv
