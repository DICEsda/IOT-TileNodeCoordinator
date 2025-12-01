# Angular Frontend Dockerfile
FROM node:18 AS build
WORKDIR /app
COPY IOT-Frontend-main/IOT-Frontend-main ./
RUN npm install && npm run build --prod

FROM nginx:alpine
COPY --from=build /app/dist /usr/share/nginx/html
COPY IOT-Frontend-main/IOT-Frontend-main/nginx.conf /etc/nginx/conf.d/default.conf
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
