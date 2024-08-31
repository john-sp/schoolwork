#!/bin/bash
python3 train_miniplaces.py
python3 eval_miniplaces.py --load ./outputs/model_best.pth.tar | head -6 | tail -1 >> results.txt
rm -rf outputs/
python3 train_miniplaces.py --batch-size 8
python3 eval_miniplaces.py --load ./outputs/model_best.pth.tar | head -6 | tail -1  >> results.txt
rm -rf outputs/
python3 train_miniplaces.py --batch-size 16
python3 eval_miniplaces.py --load ./outputs/model_best.pth.tar | head -6 | tail -1  >> results.txt
rm -rf outputs/
python3 train_miniplaces.py --lr 0.05
python3 eval_miniplaces.py --load ./outputs/model_best.pth.tar | head -6 | tail -1 >> results.txt
rm -rf outputs/
python3 train_miniplaces.py --lr 0.01
python3 eval_miniplaces.py --load ./outputs/model_best.pth.tar | head -6 | tail -1 >> results.txt
rm -rf outputs/
python3 train_miniplaces.py --epochs 20
python3 eval_miniplaces.py --load ./outputs/model_best.pth.tar | head -6 | tail -1 >> results.txt
rm -rf outputs/
python3 train_miniplaces.py --epochs 5
python3 eval_miniplaces.py --load ./outputs/model_best.pth.tar | head -6 | tail -1 >> results.txt
rm -rf outputs/