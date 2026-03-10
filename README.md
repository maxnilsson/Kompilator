# Kompilator

A compiler project built with Flex and Bison.

## Repository Structure

```
Kompilator/
├── Ass-2/
│   └── getting_started/   # Assignment 2 source code and tests
└── README.md
```

## Creating a New Folder

To create a new folder for an assignment or module, use the `mkdir` command:

```bash
mkdir Ass-3
```

Or create nested directories in one command:

```bash
mkdir -p Ass-3/getting_started
```

After creating the folder, add a `.gitkeep` file (or your source files) so Git tracks the directory:

```bash
touch Ass-3/.gitkeep
```

Then stage and commit your new folder:

```bash
git add Ass-3/
git commit -m "Add Ass-3 folder"
git push
```

## Building (Assignment 2)

```bash
cd Ass-2/getting_started
make
```

## Running

```bash
./compiler <input_file>
```

## Cleaning Build Artifacts

```bash
make clean
```
