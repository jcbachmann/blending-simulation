# Releasing

A release is an annotated git tag `vX` on a commit of `master` whose project version in [`CMakeLists.txt`](CMakeLists.txt) is `X`.
Nothing is built or published automatically: the GitHub workflows only run for pushes to `master` and for pull requests.
The project version also becomes the output of `BlendingSimulatorCli --version`.

## Version numbers

Releases are numbered `YEAR.N`, counting the releases of a year (`2026.1`, `2026.2`, ...). Right after a release the version on
`master` becomes the release version with `.1` appended (after `2026.1` it is `2026.1.1`), so that builds from `master` are told apart
from the release. The next release then takes the next number (`2026.2`), or the development version itself (`2026.1.1`) for a release
that only fixes bugs.

## Steps

1. Make sure `master` is up to date with `origin/master` and has no uncommitted changes, and that the workflows of the last push passed.
2. Set the release version in the `project(BlendingSimulator VERSION ...)` line of `CMakeLists.txt` and commit it as
   `Release vX`.
3. Tag that commit with an annotated tag `vX` whose message is `vX`.
4. Push the commit and the tag.
5. Optionally create a GitHub release from the tag (Releases, Draft a new release, choose the tag, Generate release notes).
6. Set the development version `X.1` in `CMakeLists.txt`, commit it as `Begin of development of vX.1` and push it.
7. Let dependent projects use the new tag. The Python bindings `blending_simulator_lib` in
   [blending-evaluation](https://github.com/jcbachmann/blending-evaluation) fetch this repository at the tag in the `GIT_TAG` line of
   `libraries/blending_simulator_lib/CMakeLists.txt`.

## Example: releasing v2026.2

`master` is at the development version `2026.1.1` after the release `v2026.1`. Run the commands from the root of this repository:

```bash
git switch master
git pull --ff-only
git status --short  # must print nothing

# Release commit and tag
sed -i 's/project(BlendingSimulator VERSION 2026.1.1)/project(BlendingSimulator VERSION 2026.2)/' CMakeLists.txt
git commit -am "Release v2026.2"
git tag -a v2026.2 -m "v2026.2"
git push origin master v2026.2

# Optional: GitHub release with generated notes (needs the GitHub CLI)
gh release create v2026.2 --generate-notes

# Start the next development version
sed -i 's/project(BlendingSimulator VERSION 2026.2)/project(BlendingSimulator VERSION 2026.2.1)/' CMakeLists.txt
git commit -am "Begin of development of v2026.2.1"
git push origin master
```

Check the result with `git show v2026.2:CMakeLists.txt | grep "project("`, which has to print `VERSION 2026.2`. Afterwards set
`GIT_TAG v2026.2` in `libraries/blending_simulator_lib/CMakeLists.txt` of blending-evaluation.
