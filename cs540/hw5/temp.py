import unittest
from gradescope_utils.autograder_utils.decorators import number, partial_credit, visibility, hide_errors
from subprocess import PIPE, TimeoutExpired,STDOUT, check_call, CalledProcessError
import subprocess
import numpy as np
import json
import re
import os
import pandas as pd

def read_in_output(filename):
    answer_dict = {}
    with open(filename, "r") as f:
        lines = f.read().split("\n")
        indices = [i for i, string in enumerate(lines) if "Q3a" in string]
        if len(indices) > 0:
            line_num = indices[0]+1
            current_answer = []
            while "Q" not in lines[line_num]:
                current_answer.append(lines[line_num])
                line_num += 1
            npstr = ",".join(current_answer)
            formatted = re.sub('(?<=\d|])\s+(?=\d|\[|-)',',',npstr)
            ans = np.array(json.loads(formatted))
            answer_dict["Q3a"] = ans
        indices = [i for i, string in enumerate(lines) if "Q3b" in string]
        if len(indices) > 0:
            line_num = indices[0]+1
            current_answer = []
            while "Q" not in lines[line_num]:
                current_answer.append(lines[line_num])
                line_num += 1
            npstr = ",".join(current_answer)
            formatted = re.sub('(?<=\d|])\s+(?=\d|\[|-)',',',npstr)
            ans = np.array(json.loads(formatted))
            answer_dict["Q3b"] = ans
        indices = [i for i, string in enumerate(lines) if "Q3c" in string]
        if len(indices) > 0:
            line_num = indices[0]+1
            current_answer = []
            while "Q" not in lines[line_num]:
                current_answer.append(lines[line_num])
                line_num += 1
            npstr = ",".join(current_answer)
            formatted = re.sub('(?<=\d|])\s+(?=\d|\[|-)',',',npstr)
            ans = np.array(json.loads(formatted))
            answer_dict["Q3c"] = ans
        indices = [i for i, string in enumerate(lines) if "Q3d" in string]
        if len(indices) > 0:
            line_num = indices[0]+1
            current_answer = []
            while "Q" not in lines[line_num]:
                current_answer.append(lines[line_num])
                line_num += 1
            npstr = ",".join(current_answer)
            formatted = re.sub('(?<=\d|])\s+(?=\d|\[|-)',',',npstr)
            ans = np.array(json.loads(formatted))
            answer_dict["Q3d"] = ans
        indices = [i for i, string in enumerate(lines) if "Q3e" in string]
        if len(indices) > 0:
            line_num = indices[0]+1
            current_answer = []
            while "Q" not in lines[line_num]:
                current_answer.append(lines[line_num])
                line_num += 1
            npstr = ",".join(current_answer)
            formatted = re.sub('(?<=\d|])\s+(?=\d|\[|-)',',',npstr)
            ans = np.array(json.loads(formatted))
            answer_dict["Q3e"] = ans
        indices = [i for i, string in enumerate(lines) if "Q3f" in string]
        if len(indices) > 0:
            line_num = indices[0]+1
            current_answer = []
            while "Q" not in lines[line_num]:
                current_answer.append(lines[line_num])
                line_num += 1
            npstr = ",".join(current_answer)
            formatted = re.sub('(?<=\d|])\s+(?=\d|\[|-)',',',npstr)
            ans = np.array(json.loads(formatted))
            answer_dict["Q3f"] = ans
        indices = [i for i, string in enumerate(lines) if "Q4" in string]
        if len(indices) > 0:
            if lines[indices[0]].__contains__(":"):
                A = lines[indices[0]].split(":")[1].strip()
                answer_dict["Q4"] = A
            else:
                A = lines[indices[0]].split(" ")[1].strip()
                answer_dict["Q4"] = A
        indices = [i for i, string in enumerate(lines) if "Q5a" in string]
        if len(indices) > 0:
            if lines[indices[0]].__contains__(":"):
                A = lines[indices[0]].split(":")[1].strip()
                answer_dict["Q5a"] = A
            else:
                A = lines[indices[0]].split(" ")[1].strip()
                answer_dict["Q5a"] = A
        indices = [i for i, string in enumerate(lines) if "Q5b" in string]
        if len(indices) > 0:
            if lines[indices[0]].__contains__(":"):
                A = lines[indices[0]].split(":")[1].strip()
                answer_dict["Q5b"] = A
            else:
                A = lines[indices[0]].split(" ")[1].strip()
                answer_dict["Q5b"] = A
        indices = [i for i, string in enumerate(lines) if "Q6a" in string]
        if len(indices) > 0:
            if lines[indices[0]].__contains__(":"):
                A = lines[indices[0]].split(":")[1].strip()
                answer_dict["Q6a"] = A
            else:
                A = lines[indices[0]].split(" ")[1].strip()
                answer_dict["Q6a"] = A
        indices = [i for i, string in enumerate(lines) if "Q6b" in string]
        if len(indices) > 0:
            if lines[indices[0]].__contains__(":"):
                A = lines[indices[0]].split(":")[1].strip()
                answer_dict["Q6b"] = A
            else:
                A = lines[indices[0]].split(" ")[1].strip()
                answer_dict["Q6b"] = A
    return answer_dict

def compare_answers(correct_output, student_output,total_score):
    score = total_score
    total_questions = 11
    written_questions = ["Q5a", "Q5b", "Q6b"]
    single_number_questions = ["Q4", "Q6a"]
    array_questions = ["Q3a", "Q3b","Q3c","Q3d","Q3e", "Q3f"]
    for question in sorted(correct_output.keys()):
        print(f"checking question {question}")
        if question not in student_output:
            print(f"Student did not answer {question}: -5")
            print(f'Please check output_toy.txt for expected output format.')
            score -= total_score/total_questions
        elif question in array_questions:
            try:
                student_output[question] = student_output[question].reshape(correct_output[question].shape)
                correct = np.allclose(correct_output[question], student_output[question], atol=0.001)
                if not correct:
                    print(f"{question} is incorrect -5")
                    score -= total_score/total_questions
            except ValueError:
                print(f"{question}: Error comparing arrays. Student array has shape {student_output[question].shape}, answer has shape {correct_output[question].shape}")
                score -= total_score/total_questions 
        elif question in single_number_questions:
            try:
                 student_output[question] = np.array(student_output[question]).reshape(np.array(correct_output[question]).shape)
                 if abs(float(correct_output[question]) -  float(student_output[question])) >= 0.001:
                    print(f"{question} is incorrect. Expected {float(correct_output[question])}, got {float(student_output[question])}")
                    score -= total_score/total_questions
            except ValueError:
                print(f"{question}: Error comparing arrays. Student array has shape {student_output[question].shape}, answer has shape {correct_output[question].shape}")
                score -= total_score/total_questions
        elif  question == 'Q5a':
            if '<' not in student_output[question]:
                print(f"{question} is incorrect. Expected {correct_output[question]}, got {student_output[question]}")
                score -= total_score/total_questions
    return score
            
class TestHW5(unittest.TestCase):
    def setUp(self):
        pass

    @partial_credit(30)
    @number("1")
    def test_hw5_1(self,set_score = None):
        """Train: python3 hw5.py toy.csv"""
        filename = "toy.csv"
        student_py = "hw5.py"
        student_output_file = "student_output_toy.txt"
        reference_output_file = "output_toy.txt"
        with open(student_output_file, "w") as f:
            try:
                check_call(["python3", student_py, filename], stdout=f, stderr=STDOUT)
            except Exception as e:
                print(f'Error trying to run "python3 {student_py} {filename}"')
                print(f'Your program should accept one sys.arg, the filename.')
                print(f'Error Message: {e}')
                set_score(0)
                return
        
        ref_output = read_in_output(reference_output_file)
        try: 
            student_output = read_in_output(student_output_file)
        except Exception as e:
            print(f'Error reading students answers from: "python3 {student_py} {filename}"')
            print(f'Please check the provided output_toy.txt for the expected format when you run "python3 {student_py} {filename}"')
            print(f'Error: {e}')
            set_score(1)
            return
        score = compare_answers(ref_output,student_output,25)
        with open(student_output_file, "w") as f:
            try:
                check_call(["mv", "plot.jpg","toy_plot.jpg"], stdout=f, stderr=STDOUT)
                set_score(score+5)
            except CalledProcessError as e:
                try:
                    check_call(["mv", "plot.png", "toy_plot.png"], stdout=f, stderr=STDOUT)
                    set_score(score+5)
                except CalledProcessError as e:
                    print(f"No plot detected. Maybe you forgot to save it into 'plot.jpg'?")
                    set_score(score)

    @partial_credit(20)
    @number("2")
    @hide_errors()
    def test_hw5_2(self,set_score = None):
        """Validation"""
        student_file = "hw5.csv"
        reference_file = "hw5_ref.csv"
        if not os.path.isfile(student_file):
            set_score(0)
        df_student = pd.read_csv(student_file)
        df_reference = pd.read_csv(reference_file)
        headers_student = list(df_student.columns)
        headers_reference = list(df_reference.columns)
        if headers_student != headers_reference and len(df_student) > 10:
            set_score(1)
        elif df_student.shape != df_reference.shape:
            set_score(5)
        elif not df_student.equals(df_reference):
            set_score(10)
        else:
            set_score(20)

    @partial_credit(50)
    @number("3")
    @visibility("after_due_date")
    def test_hw5_3(self,set_score = None):
        """Test: python3 hw5.py hw5.csv""" #shape of output from hw5.csv
        filename = "hw5_ref.csv"
        student_py = "hw5.py"
        student_output_file = "output_student_hw5.txt"
        reference_output_file = "output_hw5.txt"
        with open(student_output_file, "w") as f:
            try:
                check_call(["python3", student_py, filename], stdout=f, stderr=STDOUT)
            except Exception as e:
                print(f'Error trying to run "python3 {student_py} {filename}"')
                print(f'Your program should accept one sys.arg,the filename.')
                print(f'Error Message: {e}')
                set_score(0)
                return
            ref_output = read_in_output(reference_output_file)
        try: 
            student_output = read_in_output(student_output_file)
        except Exception as e:
            print(f'Error reading students answers from: "python3 {student_py} {filename}"')
            print(f'Please check the provided output_toy.txt for the expected format when you run "python3 {student_py} {filename}"')
            print(f'Error: {e}')
            set_score(1)
            return
        score = compare_answers(ref_output,student_output,42)
        with open(student_output_file, "w") as f:
            try:
                check_call(["mv", "plot.jpg","toy_plot.jpg"], stdout=f, stderr=STDOUT)
                set_score(score+8)
            except CalledProcessError as e:
                try:
                    check_call(["mv", "plot.png", "toy_plot.png"], stdout=f, stderr=STDOUT)
                    set_score(score+8)
                except CalledProcessError as e:
                    print(f"No plot detected. Maybe you forgot to save it into 'plot.jpg'?")
                    set_score(score)
