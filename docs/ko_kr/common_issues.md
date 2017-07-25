# Common Issues
**Translation**: [English](../common_issues.md)

## 리소스 혹은 플러그인 로드 실패 (Windows)
경로가 정확함에도 불구하고 리소스나 플러그인 로드가 실패한다면, 경로의 길이가 너무 긴 경우일 수 있다.
윈도우즈에서는 경로 길이가 최대 260자까지 허용하기 때문에, 이를 넘는다면 전체 경로를 줄이거나 상대경로를 활용해야 한다.

윈도우 10부터는 운영체제에서 최대 경로 길이 제한을 늘릴 수 있는 기능을 제공한다. 이는 컴파일 옵션 수정과
윈도우 기능 수정을 필요로 한다. 자세한 내용은 다음 경로를 참조: ...