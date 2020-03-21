from setuptools import setup, find_packages

setup(
    name='libaps2',
    version="1.3",
    url='https://github.com/BBN-Q/libaps2',
    py_modules=["aps2"],
    install_requires=[
        'numpy>=1.18'
    ]
)
